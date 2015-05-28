/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 University of Pennsylvania
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "penn-chord.h"

#include "ns3/random-variable.h"
#include "ns3/inet-socket-address.h"

using namespace ns3;

float PennChord::globalHopCount = 0;
float PennChord::globalQueryCount = 0;

TypeId
PennChord::GetTypeId ()
{
  static TypeId tid = TypeId ("PennChord")
    .SetParent<PennApplication> ()
    .AddConstructor<PennChord> ()
    .AddAttribute ("AppPort",
                   "Listening port for Application",
                   UintegerValue (10001),
                   MakeUintegerAccessor (&PennChord::m_appPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PingTimeout",
                   "Timeout value for PING_REQ in milliseconds",
                   TimeValue (MilliSeconds (2000)),
                   MakeTimeAccessor (&PennChord::m_pingTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("StabilizePeriod",
                   "Timeout value for Stabilize in milliseconds",
                   TimeValue (MilliSeconds (5000)),
                   MakeTimeAccessor (&PennChord::m_stabilizeTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("FixFingerPeriod",
                   "Timeout value for fixing finger table in milliseconds",
                   TimeValue (MilliSeconds (8000)),
                   MakeTimeAccessor (&PennChord::m_fixFingerTimeout),
                   MakeTimeChecker ())
        ;
  return tid;
}

PennChord::PennChord ()
  : m_auditPingsTimer (Timer::CANCEL_ON_DESTROY)
{
  RandomVariable random;
  SeedManager::SetSeed (time (NULL));
  random = UniformVariable (0x00000000, 0xFFFFFFFF);
  m_currentTransactionId = random.GetInteger ();
}

PennChord::~PennChord ()
{

}

void
PennChord::DoDispose ()
{
  StopApplication ();
  PennApplication::DoDispose ();
}

void
PennChord::StartApplication (void)
{
  if (m_socket == 0)
    { 
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny(), m_appPort);
      m_socket->Bind (local);
      m_socket->SetRecvCallback (MakeCallback (&PennChord::RecvMessage, this));
    }  

  m_chordStatus=0;
  m_localAddress = GetLocalAddress();
  SHA_1(m_localAddress, m_localDigest);
  SetSuccessorAddress (Ipv4Address::GetAny());
  SetPredecessorAddress (Ipv4Address::GetAny());

  // Configure timers
  m_auditPingsTimer.SetFunction (&PennChord::AuditPings, this);
  m_stabilizeTimer.SetFunction (&PennChord::Stabilize,this);
  m_fixFingerTimer.SetFunction (&PennChord::FixFinger,this);
  // Start timers
  m_auditPingsTimer.Schedule (m_pingTimeout);
  m_stabilizeTimer.Schedule (m_stabilizeTimeout);
  m_fixFingerTimer.Schedule (m_fixFingerTimeout);
  //DoTest();
}

void
PennChord::StopApplication (void)
{
  // Close socket
  if (m_socket)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  PRINT_LOG("------------------Average Hop Count ="<< (PennChord::globalHopCount/PennChord::globalQueryCount)<<"------------------");

  // Cancel timers
  m_auditPingsTimer.Cancel ();

  m_pingTracker.clear ();
  m_stabilizeTimer.Cancel();
  m_fixFingerTimer.Cancel();
}

void
PennChord::ProcessCommand (std::vector<std::string> tokens)
{
  std::vector<std::string>::iterator iterator = tokens.begin();
  std::string command = *iterator;
  if (command == "JOIN")
  {
    iterator++;  
    std::string nodeId = *iterator;
    Ipv4Address nodeAddr = ResolveNodeIpAddress(nodeId);

    //if the chord status in on, print an error message saying that you are already in a chord
    if (m_chordStatus == 1)
    {
      ERROR_LOG("The node trying to join is already in a chord");
      return;
    }
    //if the address at the incremented iterator is the its own address, then check the chordstatus
    if (nodeAddr == m_localAddress)
    {
      m_chordStatus=1;
      SetSuccessorAddress(m_localAddress);
      SetPredecessorAddress(Ipv4Address::GetAny());
      CHORD_LOG ("Chord is created with a landmark node: " << nodeId );
    }

    else
    {
      //send a joinchord packet to the particular node/address and wait for the reply
      //the reply is either a yes with the suggested successor
      //or no if the node itself isn't in a chord
            
      if (nodeAddr != Ipv4Address::GetAny ())
      {
        uint32_t transactionId = GetNextTransactionId ();
        //CHORD_LOG ("Sending JOIN_CHORD to Node: " << ReverseLookup(nodeAddr) << " IP: " << nodeAddr << " transactionId: " << transactionId);
        Ptr<Packet> packet = Create<Packet> ();
        PennChordMessage message = PennChordMessage (PennChordMessage::JOIN_CHORD, transactionId);
        packet->AddHeader (message);
        m_socket->SendTo (packet, 0 , InetSocketAddress (nodeAddr, m_appPort));
      }
      else
      {
        ERROR_LOG("Invalid node ID");
      }
    }
  }

  if (command == "LEAVE")
  {
      if (m_chordStatus == 0)
      {
        ERROR_LOG("Cannot LEAVE since the node isn't in a chord");
        return;
      }
      if (m_successorAddr == m_localAddress)
      {
          m_chordStatus =0;
          SetSuccessorAddress (Ipv4Address::GetAny());
          SetPredecessorAddress (Ipv4Address::GetAny());
          m_fingerTable.clear();
          return;
      }
      uint32_t stransactionId = GetNextTransactionId ();
      Ptr<Packet> spacket = Create<Packet> ();
      PennChordMessage smessage = PennChordMessage (PennChordMessage::LEAVE_SUCCESSOR, stransactionId);
      smessage.SetLeaveSuccessor (m_predecessorAddr);
      if (m_successorAddr == m_predecessorAddr)
      {
          smessage.SetLeaveSuccessor (Ipv4Address::GetAny());
      }
      spacket->AddHeader (smessage);
      m_socket->SendTo (spacket, 0 , InetSocketAddress (m_successorAddr, m_appPort));
      m_chordLeaveNotifyFn (m_successorAddr, stransactionId);

      uint32_t ptransactionId = GetNextTransactionId ();
      Ptr<Packet> ppacket = Create<Packet> ();
      PennChordMessage pmessage = PennChordMessage (PennChordMessage::LEAVE_PREDECESSOR, ptransactionId);
      pmessage.SetLeavePredecessor (m_successorAddr);
      ppacket->AddHeader (pmessage);
      m_socket->SendTo (ppacket, 0 , InetSocketAddress (m_predecessorAddr, m_appPort));

      m_chordStatus =0;
      SetSuccessorAddress (Ipv4Address::GetAny());
      SetPredecessorAddress (Ipv4Address::GetAny());
      m_fingerTable.clear();
  }
  if (command == "RINGSTATE")
  {
    if (m_chordStatus == 0)
    {
      ERROR_LOG("Cannot initiate Ringstate since it isn't in a chord");
      return;
    }
    CHORD_LOG ("RINGSTATE START");
    DisplayChordDetails();
    uint32_t transactionId = GetNextTransactionId ();
    Ptr<Packet> packet = Create<Packet> ();
    PennChordMessage message = PennChordMessage (PennChordMessage::RINGSTATE, transactionId);
    message.SetRingstate (m_localAddress);
    packet->AddHeader (message);
    m_socket->SendTo (packet, 0 , InetSocketAddress (m_successorAddr, m_appPort));
  }
  if (command == "FINGERS")
  {
      CHORD_LOG("-------------Finger Table of "<<DisplayHEX(m_localDigest));
      unsigned char indexHash[20];
      for (std::map<uint16_t, Ipv4Address>::iterator iter = m_fingerTable.begin();iter!=m_fingerTable.end();iter++)
      {
          FindIndexHash(iter->first,indexHash);
          PRINT_LOG ("\t"<<"Index : "<<DisplayHEX(indexHash)<<" Successor : "<< ReverseLookup(iter->second));
      }
  }

}

void
PennChord::SendPing (Ipv4Address destAddress, std::string pingMessage)
{
  if (destAddress != Ipv4Address::GetAny ())
    {
      uint32_t transactionId = GetNextTransactionId ();
      CHORD_LOG ("Sending PING_REQ to Node: " << ReverseLookup(destAddress) << " IP: " << destAddress << " Message: " << pingMessage << " transactionId: " << transactionId);
      Ptr<PingRequest> pingRequest = Create<PingRequest> (transactionId, Simulator::Now(), destAddress, pingMessage);
      // Add to ping-tracker
      m_pingTracker.insert (std::make_pair (transactionId, pingRequest));
      Ptr<Packet> packet = Create<Packet> ();
      PennChordMessage message = PennChordMessage (PennChordMessage::PING_REQ, transactionId);
      message.SetPingReq (pingMessage);
      packet->AddHeader (message);
      m_socket->SendTo (packet, 0 , InetSocketAddress (destAddress, m_appPort));
    }
  else
    {
      // Report failure   
      m_pingFailureFn (destAddress, pingMessage);
    }
}

void
PennChord::RecvMessage (Ptr<Socket> socket)
{
  Address sourceAddr;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddr);
  InetSocketAddress inetSocketAddr = InetSocketAddress::ConvertFrom (sourceAddr);
  Ipv4Address sourceAddress = inetSocketAddr.GetIpv4 ();
  uint16_t sourcePort = inetSocketAddr.GetPort ();
  PennChordMessage message;
  packet->RemoveHeader (message);

  switch (message.GetMessageType ())
    {
      case PennChordMessage::PING_REQ:
        ProcessPingReq (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::PING_RSP:
        ProcessPingRsp (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::JOIN_CHORD:
        ProcessJoinChord (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::JOIN_CHORD_FAIL:
        ProcessJoinChordFail (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::JOIN_CHORD_SUCCESS:
        ProcessJoinChordSuccess (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::FIND_SUCCESSOR:
        ProcessFindSuccessor (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::NOTIFY:
        ProcessNotify (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::STABILIZE_REQ:
        ProcessStabilizeReq (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::STABILIZE_RESP:
        ProcessStabilizeResp (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::RINGSTATE:
        ProcessRingstate (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::LEAVE_SUCCESSOR:
        ProcessLeaveSuccessor (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::LEAVE_PREDECESSOR:
        ProcessLeavePredecessor (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::FIND_FINGER:
        ProcessFindFinger (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::FIND_FINGER_SUCCESS:
        ProcessFindFingerSuccess (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::LOOKUP_PUBLISH:
        ProcessLookupPublish (message, sourceAddress, sourcePort);
        break;
      case PennChordMessage::LOOKUP_PUBLISH_SUCCESS:
        ProcessLookupPublishSuccess (message, sourceAddress, sourcePort);
        break;
      default:
        ERROR_LOG ("Unknown Message Type!");
        break;
    }
}

void
PennChord::ProcessPingReq (PennChordMessage message, Ipv4Address sourceAddress, uint16_t sourcePort)
{

    // Use reverse lookup for ease of debug
    std::string fromNode = ReverseLookup (sourceAddress);
    CHORD_LOG ("Received PING_REQ, From Node: " << fromNode << ", Message: " << message.GetPingReq().pingMessage);
    // Send Ping Response
    PennChordMessage resp = PennChordMessage (PennChordMessage::PING_RSP, message.GetTransactionId());
    resp.SetPingRsp (message.GetPingReq().pingMessage);
    Ptr<Packet> packet = Create<Packet> ();
    packet->AddHeader (resp);
    m_socket->SendTo (packet, 0 , InetSocketAddress (sourceAddress, sourcePort));
    // Send indication to application layer
    m_pingRecvFn (sourceAddress, message.GetPingReq().pingMessage);
}

void
PennChord::ProcessPingRsp (PennChordMessage message, Ipv4Address sourceAddress, uint16_t sourcePort)
{
  // Remove from pingTracker
  std::map<uint32_t, Ptr<PingRequest> >::iterator iter;
  iter = m_pingTracker.find (message.GetTransactionId ());
  if (iter != m_pingTracker.end ())
    {
      std::string fromNode = ReverseLookup (sourceAddress);
      CHORD_LOG ("Received PING_RSP, From Node: " << fromNode << ", Message: " << message.GetPingRsp().pingMessage);
      m_pingTracker.erase (iter);
      // Send indication to application layer
      m_pingSuccessFn (sourceAddress, message.GetPingRsp().pingMessage);
    }
  else
    {
      DEBUG_LOG ("Received invalid PING_RSP!");
    }
}

void
PennChord::AuditPings ()
{
  std::map<uint32_t, Ptr<PingRequest> >::iterator iter;
  for (iter = m_pingTracker.begin () ; iter != m_pingTracker.end();)
    {
      Ptr<PingRequest> pingRequest = iter->second;
      if (pingRequest->GetTimestamp().GetMilliSeconds() + m_pingTimeout.GetMilliSeconds() <= Simulator::Now().GetMilliSeconds())
        {
          DEBUG_LOG ("Ping expired. Message: " << pingRequest->GetPingMessage () << " Timestamp: " << pingRequest->GetTimestamp().GetMilliSeconds () << " CurrentTime: " << Simulator::Now().GetMilliSeconds ());
          // Remove stale entries
          m_pingTracker.erase (iter++);
          // Send indication to application layer
          m_pingFailureFn (pingRequest->GetDestinationAddress(), pingRequest->GetPingMessage ());
        }
      else
        {
          ++iter;
        }
    }
  // Rechedule timer
  m_auditPingsTimer.Schedule (m_pingTimeout); 
}

void
PennChord::ProcessJoinChord (PennChordMessage message, Ipv4Address sourceAddress, uint16_t sourcePort)
{
  if(m_chordStatus ==0)
  {
    std::string fromNode = ReverseLookup (sourceAddress);
    //CHORD_LOG ("Received JOIN_CHORD, From Node: " << fromNode );
    PennChordMessage resp = PennChordMessage (PennChordMessage::JOIN_CHORD_FAIL, message.GetTransactionId());
    Ptr<Packet> packet = Create<Packet> ();
    packet->AddHeader (resp);
    m_socket->SendTo (packet, 0 , InetSocketAddress (sourceAddress, sourcePort));
    return;
  }
  unsigned char m_recvDigest[20];
  SHA_1(sourceAddress, m_recvDigest);
  CHORD_LOG (" LookupIssue<["<<DisplayHEX(m_localDigest)<<"], ["<<DisplayHEX(m_recvDigest)<<"]>");
  if (m_localAddress == m_successorAddr)
  {
    SetSuccessorAddress(sourceAddress); 
    ReplyFindSuccessor (message, sourceAddress, sourcePort, m_localAddress);
    SendNotify(message, m_successorAddr,sourcePort);
    return;
  }
  FindSuccessor(message, sourceAddress, sourcePort, m_recvDigest);
}

void 
PennChord::ProcessFindSuccessor (PennChordMessage message, Ipv4Address sourceAddress, uint16_t sourcePort)
{
  std::string fromNode = ReverseLookup (sourceAddress);
  //CHORD_LOG ("Received FIND_SUCCESSOR, From Node: " << fromNode);
  Ipv4Address destAddr = message.GetFindSuccessor().destAddress;
  unsigned char m_digest[20];
  memcpy(m_digest,message.GetFindSuccessor().targetDigest, 20);
  FindSuccessor(message, destAddr, sourcePort, m_digest);
}


void
PennChord::FindSuccessor (PennChordMessage message, Ipv4Address destAddr, uint16_t sourcePort, unsigned char *m_digest)
{
  //compare current node's SHA with its successor's SHA
  //unsigned char m_digest[20];
  //SHA_1(destAddr, m_digest);
  if (compareSHA1(m_localDigest,m_successorDigest))
  {
    if (compareSHA1(m_digest,m_localDigest) || compareSHA1 (m_successorDigest,m_digest))
    {
      ReplyFindSuccessor (message, destAddr, sourcePort, m_successorAddr);
      return;
    }
    SendFindSuccessor (message, destAddr, sourcePort, m_digest);
    return;
  }
  if (compareSHA1(m_successorDigest, m_digest) && compareSHA1(m_digest, m_localDigest))
  {
    ReplyFindSuccessor (message, destAddr, sourcePort, m_successorAddr);
    return;
  }
  SendFindSuccessor (message, destAddr,sourcePort, m_digest);
}

void
PennChord::SendFindSuccessor (PennChordMessage message, Ipv4Address destAddr, uint16_t sourcePort, unsigned char *targetDigest)
{
    //unsigned char targetDigest[20];
    //SHA_1(destAddr, targetDigest);
    Ipv4Address NextHopAddr;
    unsigned char NextHopDigest[20];
    NextHopAddr = FindNextHop (targetDigest);
    SHA_1(NextHopAddr, NextHopDigest);

    CHORD_LOG (" LookupRequest<["<<DisplayHEX(m_localDigest)<<"]: NextHop<"<<NextHopAddr<<", ["<<DisplayHEX(NextHopDigest)<<"], ["<<DisplayHEX(targetDigest)<<"]>");
    //CHORD_LOG ("Sending FIND_SUCCESSOR to Node: " << ReverseLookup(m_successorAddr) << " IP: " << m_successorAddr <<" transactionId: " << message.GetTransactionId());
    Ptr<Packet> packet = Create<Packet> ();
    PennChordMessage newMessage = PennChordMessage (PennChordMessage::FIND_SUCCESSOR,message.GetTransactionId());
    newMessage.SetFindSuccessor (destAddr, targetDigest);
    packet->AddHeader (newMessage);
    m_socket->SendTo (packet, 0 , InetSocketAddress (NextHopAddr,sourcePort));
}

void
PennChord::ReplyFindSuccessor (PennChordMessage message, Ipv4Address destAddr, uint16_t sourcePort, Ipv4Address successorAddr)
{
    unsigned char targetDigest[20];
    SHA_1(destAddr, targetDigest);
    CHORD_LOG (" LookupResult<["<<DisplayHEX(m_localDigest)<<"], ["<<DisplayHEX(targetDigest)<<"], "<<ReverseLookup(destAddr)<<">");
    //CHORD_LOG ("Sending JOIN_CHORD_SUCCESS to Node: " << ReverseLookup(destAddr) << " IP: " << destAddr <<" transactionId: " << message.GetTransactionId());
    Ptr<Packet> packet = Create<Packet> ();
    PennChordMessage newMessage = PennChordMessage (PennChordMessage::JOIN_CHORD_SUCCESS,message.GetTransactionId());
    newMessage.SetJoinChordSuccess (successorAddr);
    packet->AddHeader (newMessage);
    m_socket->SendTo (packet, 0 , InetSocketAddress (destAddr,sourcePort));
}

void
PennChord::ProcessJoinChordSuccess (PennChordMessage message, Ipv4Address sourceAddress, uint16_t sourcePort)
{
    //std::string fromNode = ReverseLookup (sourceAddress);
    //CHORD_LOG ("Received JOIN_CHORD_SUCCESS, From Node: " << fromNode);
    Ipv4Address successorAddr = message.GetJoinChordSuccess().successorAddress;
    m_chordStatus =1;
    SetSuccessorAddress (successorAddr);
    SetPredecessorAddress (Ipv4Address::GetAny ());
    //CHORD_LOG ("PREDECESSOR: " << m_predecessorAddr << " SUCCESSOR " << m_successorAddr);
    SendNotify(message, m_successorAddr,sourcePort);
}

void
PennChord::ProcessJoinChordFail (PennChordMessage message, Ipv4Address sourceAddress, uint16_t sourcePort)
{
    ERROR_LOG ("The node you approached in not in a chord");
}

void
PennChord::SendNotify(PennChordMessage message, Ipv4Address destAddr, int16_t sourcePort)
{
    //CHORD_LOG ("PREDECESSOR: " << m_predecessorAddr << " SUCCESSOR " << m_successorAddr);
    //CHORD_LOG ("Sending NOTIFY to Node: " << ReverseLookup(destAddr) << " IP: " << destAddr <<" transactionId(): " << message.GetTransactionId());
    Ptr<Packet> packet = Create<Packet> ();
    PennChordMessage newMessage = PennChordMessage (PennChordMessage::NOTIFY,message.GetTransactionId());
    packet->AddHeader (newMessage);
    m_socket->SendTo (packet, 0 , InetSocketAddress (destAddr,sourcePort));
}

void
PennChord::ProcessNotify(PennChordMessage message, Ipv4Address sourceAddress, uint16_t sourcePort)
{
    std::string fromNode = ReverseLookup(sourceAddress);
    //CHORD_LOG ("Recieved NOTIFY from Node: " << fromNode << " IP: " << sourceAddress <<" transactionId: " << message.GetTransactionId());
    if (m_predecessorAddr == Ipv4Address::GetAny())
    {
       SetPredecessorAddress (sourceAddress);
       //m_chordJoinNotifyFn(sourceAddress, message.GetTransactionId());
       return;
    }
    unsigned char m_recvDigest[20];
    SHA_1(sourceAddress, m_recvDigest);
    if (compareSHA1(m_predecessorDigest, m_localDigest))
    {
      if (compareSHA1(m_recvDigest, m_predecessorDigest) || compareSHA1(m_localDigest, m_recvDigest))
      {
        SetPredecessorAddress (sourceAddress);
        m_chordJoinNotifyFn(sourceAddress, message.GetTransactionId());
        return;
      }
    }
    else if (compareSHA1(m_recvDigest, m_predecessorDigest) && compareSHA1(m_localDigest, m_recvDigest))
    {
      SetPredecessorAddress (sourceAddress);
      m_chordJoinNotifyFn(sourceAddress, message.GetTransactionId());
      return;
    }
    return;
    //CHORD_LOG ("PREDECESSOR: " << m_predecessorAddr << " SUCCESSOR " << m_successorAddr);
}

void
PennChord::Stabilize()
{
    if (m_chordStatus==1 && m_successorAddr != m_localAddress)
          {
            uint32_t transactionId = GetNextTransactionId ();
            //CHORD_LOG ("PREDECESSOR: " << m_predecessorAddr << " SUCCESSOR " << m_successorAddr);
            //CHORD_LOG ("Sending STABILIZE_REQ to Node: " << ReverseLookup(m_successorAddr) << " IP: " << m_successorAddr << " transactionId: " << transactionId);
            Ptr<Packet> packet = Create<Packet> ();
            PennChordMessage newmessage = PennChordMessage (PennChordMessage::STABILIZE_REQ, transactionId);
            packet->AddHeader (newmessage);
            m_socket->SendTo (packet, 0 , InetSocketAddress (m_successorAddr, m_appPort));

          }
    // Reschedule Timer
   m_stabilizeTimer.Schedule (m_stabilizeTimeout);
    return;
}


void
PennChord::ProcessStabilizeReq (PennChordMessage message, Ipv4Address sourceAddress, int16_t sourcePort)
{
    if(m_predecessorAddr != Ipv4Address::GetAny())
    {
        //CHORD_LOG ("Recieved STABILIZE_REQ from Node: " << ReverseLookup(sourceAddress) << " IP: " << sourceAddress << " transactionId: " << message.GetTransactionId());
        //CHORD_LOG ("PREDECESSOR: " << m_predecessorAddr << " SUCCESSOR " << m_successorAddr);
        Ptr<Packet> packet = Create<Packet> ();
        PennChordMessage newmessage = PennChordMessage (PennChordMessage::STABILIZE_RESP, message.GetTransactionId());
        newmessage.SetStabilizeResp (m_predecessorAddr);
        packet->AddHeader (newmessage);
        m_socket->SendTo (packet, 0 , InetSocketAddress (sourceAddress, sourcePort));
     }
    return;
}

void
PennChord::ProcessStabilizeResp (PennChordMessage message, Ipv4Address sourceAddress, int16_t sourcePort)
{

    //CHORD_LOG ("Recieved STABILIZE_RESP from Node: " << ReverseLookup(sourceAddress) << " IP: " << sourceAddress << " transactionId: " << message.GetTransactionId());
    Ipv4Address predecessorAddr=message.GetStabilizeResp().predecessorAddress;
    if (predecessorAddr == Ipv4Address::GetAny())
    {
        return;
    }
    unsigned char m_recvdigest[20];
    SHA_1(predecessorAddr,m_recvdigest);

    if(compareSHA1(m_recvdigest,m_localDigest)==2)
    {
        return;
    }

    if(compareSHA1(m_localDigest,m_successorDigest))
    {
        if(compareSHA1(m_recvdigest,m_localDigest) || compareSHA1(m_successorDigest, m_recvdigest))
        {
            SetSuccessorAddress (predecessorAddr);
            SendNotify(message,m_successorAddr,sourcePort);
        }
        return;
    }

    if(compareSHA1(m_recvdigest,m_localDigest) && compareSHA1(m_successorDigest, m_recvdigest))
    {
        SetSuccessorAddress (predecessorAddr);
        SendNotify(message,m_successorAddr,sourcePort);
    }
        //CHORD_LOG ("PREDECESSOR: " << m_predecessorAddr << " SUCCESSOR " << m_successorAddr);
        return;
}

void
PennChord::ProcessRingstate (PennChordMessage message, Ipv4Address sourceAddress, int16_t sourcePort)
{
    Ipv4Address InitiatorAddr = message.GetRingstate().initiatorAddress;
    if ( InitiatorAddr == m_localAddress )
        return;
    //CHORD_LOG ("Recieved RINGSTATE from Node: " << ReverseLookup(sourceAddress) << " IP: " << sourceAddress << " transactionId: " << message.GetTransactionId());
    DisplayChordDetails();
    Ptr<Packet> packet = Create<Packet> ();
    PennChordMessage newmessage = PennChordMessage (PennChordMessage::RINGSTATE, message.GetTransactionId());
    newmessage.SetRingstate (InitiatorAddr);
    packet->AddHeader (newmessage);
    m_socket->SendTo (packet, 0 , InetSocketAddress (m_successorAddr, sourcePort));
}

void
PennChord::DisplayChordDetails ()
{
    CHORD_LOG ("-------------------------------------RING STATE----------------------------------");
    PRINT_LOG ("Current Node Number:     "<<ReverseLookup(m_localAddress)   <<" IP: "<<m_localAddress   << " ID: "<<DisplayHEX(m_localDigest));
    PRINT_LOG ("Successor Node Number:   "<<ReverseLookup(m_successorAddr)  <<" IP: "<<m_successorAddr  << " ID: "<<DisplayHEX(m_successorDigest));
    PRINT_LOG ("Predecessor Node Number: "<<ReverseLookup(m_predecessorAddr)<<" IP: "<<m_predecessorAddr<< " ID: "<<DisplayHEX(m_predecessorDigest));
    return;
}

void
PennChord::ProcessLeaveSuccessor (PennChordMessage message, Ipv4Address sourceAddress, int16_t sourcePort)
{
    //CHORD_LOG ("Recieved LEAVE_SUCCESSOR from Node: " << ReverseLookup(sourceAddress) << " IP: " << sourceAddress << " transactionId: " << message.GetTransactionId());
    SetPredecessorAddress (message.GetLeaveSuccessor().predecessorAddress);
}

void
PennChord::ProcessLeavePredecessor (PennChordMessage message, Ipv4Address sourceAddress, int16_t sourcePort)
{
    //CHORD_LOG ("Recieved LEAVE_PREDECESSOR from Node: " << ReverseLookup(sourceAddress) << " IP: " << sourceAddress << " transactionId: " << message.GetTransactionId());
    SetSuccessorAddress (message.GetLeavePredecessor().successorAddress);
}

void
PennChord::SetSuccessorAddress(Ipv4Address ipv4Address)
{
  m_successorAddr = ipv4Address;
  SHA_1(m_successorAddr, m_successorDigest);
}

void
PennChord::SetPredecessorAddress(Ipv4Address ipv4Address)
{
  m_predecessorAddr = ipv4Address;
  SHA_1(m_predecessorAddr, m_predecessorDigest);
}


void
PennChord::FixFinger()
{
    if (m_chordStatus==0 || m_successorAddr == m_localAddress)
    {
        m_fixFingerTimer.Schedule (m_fixFingerTimeout);
        return;
    }
    m_fingerTable[1] = m_successorAddr;
    SendFindFinger(2);
}

void
PennChord::SendFindFinger (uint16_t i)
{
    unsigned char indexHash[20];
    unsigned char prevIndexHash[20];
    Ipv4Address prevFinger;
    unsigned char prevFingerHash[20];

    while(1)
    {
        if (i>160)
        {
            m_fixFingerTimer.Schedule (m_fixFingerTimeout);
            return;
        }

        FindIndexHash (i,indexHash);
        FindIndexHash (i-1,prevIndexHash);
        prevFinger = m_fingerTable.find(i-1)->second;
        SHA_1(prevFinger, prevFingerHash);
        if(compareSHA1(prevIndexHash,prevFingerHash))
        {
            //PRINT_LOG("FIND_FINGER entering corner case");
            if(compareSHA1(prevFingerHash,indexHash) || compareSHA1(indexHash,prevIndexHash))
            {
                m_fingerTable[i]=prevFinger;
                i++;
                continue;
            }
        }
        else if(compareSHA1(prevFingerHash,indexHash) && compareSHA1(indexHash,prevIndexHash))
        {
            m_fingerTable[i]=prevFinger;
            i++;
            continue;
        }
        break;
    }
    uint32_t transactionId = GetNextTransactionId ();
    //CHORD_LOG ("Sending FIND_FINGER to Node: " << ReverseLookup(prevFinger) << " IP: " << prevFinger <<" transactionId: " << transactionId << " INDEX : " << i);
    Ptr<Packet> packet = Create<Packet> ();
    PennChordMessage message = PennChordMessage (PennChordMessage::FIND_FINGER,transactionId);
    message.SetFindFinger (m_localAddress, indexHash, i);
    packet->AddHeader (message);
    m_socket->SendTo (packet, 0 , InetSocketAddress (prevFinger,m_appPort));
}

void
PennChord::ProcessFindFinger (PennChordMessage message, Ipv4Address sourceAddress, uint16_t sourcePort)
{
    //std::string fromNode = ReverseLookup (sourceAddress);
    //CHORD_LOG ("Received FIND_FINGER, From Node: " << fromNode);
    Ipv4Address targetAddr = message.GetFindFinger().targetAddress;
    unsigned char targetDig[20];
    memcpy(targetDig,message.GetFindFinger().targetDigest, 20);
    uint16_t targetInd = message.GetFindFinger().targetIndex;

    if (compareSHA1(m_localDigest,m_successorDigest))
    {
      if (compareSHA1(targetDig,m_localDigest) || compareSHA1 (m_successorDigest,targetDig))
      {
        ReplyFindFinger (message, targetAddr, sourcePort, targetInd);
        return;
      }
    }
    else if (compareSHA1(m_successorDigest, targetDig) && compareSHA1(targetDig, m_localDigest))
    {
      ReplyFindFinger (message, targetAddr, sourcePort, targetInd);
      return;
    }
    Ptr<Packet> packet = Create<Packet> ();
    PennChordMessage newMessage = PennChordMessage (PennChordMessage::FIND_FINGER,message.GetTransactionId());
    newMessage.SetFindFinger (targetAddr, targetDig, targetInd);
    packet->AddHeader (newMessage);
    Ipv4Address NextHopAddr = FindNextHop (targetDig);
    m_socket->SendTo (packet, 0 , InetSocketAddress (NextHopAddr,sourcePort));
}

void
PennChord::ReplyFindFinger(PennChordMessage message, Ipv4Address targetAddr, uint16_t sourcePort,uint16_t targetInd)
{
    Ptr<Packet> packet = Create<Packet> ();
    PennChordMessage newMessage = PennChordMessage (PennChordMessage::FIND_FINGER_SUCCESS,message.GetTransactionId());
    newMessage.SetFindFingerSuccess (m_successorAddr,targetInd);
    packet->AddHeader (newMessage);
    m_socket->SendTo (packet, 0 , InetSocketAddress (targetAddr,sourcePort));
}

void
PennChord::ProcessFindFingerSuccess (PennChordMessage message, Ipv4Address sourceAddress, uint16_t sourcePort)
{
    std::string fromNode = ReverseLookup (sourceAddress);
    //CHORD_LOG ("Received FIND_FINGER_SUCCESS, From Node: " << fromNode);
    uint16_t fingerInd = message.GetFindFingerSuccess().fingerIndex;
    m_fingerTable[fingerInd]=message.GetFindFingerSuccess().fingerAddress;
    SendFindFinger(fingerInd+1);
}

Ipv4Address
PennChord::FindNextHop (unsigned char *targetDigest)
{
    //return m_successorAddr;
    unsigned char fingerDigest[20];
    for (std::map<uint16_t, Ipv4Address>::iterator iter = (--m_fingerTable.end());iter!=m_fingerTable.begin();iter--)
    {
        if(m_localAddress == iter->second)
            continue;
        SHA_1(iter->second,fingerDigest);
        if (compareSHA1(m_localDigest,targetDigest))
        {
          if (compareSHA1(targetDigest,fingerDigest) || compareSHA1 (fingerDigest,m_localDigest))
          {
            return iter->second;
          }
        }
        else if (compareSHA1(targetDigest, fingerDigest) && compareSHA1(fingerDigest, m_localDigest))
        {
          return iter->second;
        }
    }
    //CHORD_LOG("Reached end of finger table");
    return m_successorAddr;
}

void
PennChord::LookupPublish (std::string key, uint16_t flag, uint32_t transactionId)
{
    unsigned char digestkey[20];
    SHA_1(key,digestkey);
    CHORD_LOG (" LookupIssue<["<<DisplayHEX(m_localDigest)<<"], ["<<DisplayHEX(digestkey)<<"]>");

    if (compareSHA1(m_localDigest,m_successorDigest))
    {
      if (compareSHA1(digestkey,m_localDigest) || compareSHA1 (m_successorDigest,digestkey))
      {
          LookupCallback(flag, key, m_successorAddr,transactionId);
      }
    }
    if (compareSHA1(m_successorDigest, digestkey) && compareSHA1(digestkey, m_localDigest))
    {
        LookupCallback(flag, key, m_successorAddr,transactionId);
    }
    PennChord::globalQueryCount++;
    PennChord::globalHopCount++;

    unsigned char NextHopDigest[20];
    Ipv4Address NextHopAddr = FindNextHop (digestkey);
    SHA_1(NextHopAddr, NextHopDigest);
    CHORD_LOG (" LookupRequest<["<<DisplayHEX(m_localDigest)<<"]: NextHop<"<< NextHopAddr <<", ["<<DisplayHEX(NextHopDigest)<<"], ["<<DisplayHEX(digestkey)<<"]>");
    Ptr<Packet> packet = Create<Packet> ();
    PennChordMessage message = PennChordMessage (PennChordMessage::LOOKUP_PUBLISH, transactionId);
    message.SetLookupPublish (flag, m_localAddress, digestkey, key);
    packet->AddHeader (message);
    m_socket->SendTo (packet, 0 , InetSocketAddress (NextHopAddr, m_appPort));
    return;
}

void
PennChord::ProcessLookupPublish (PennChordMessage message, Ipv4Address sourceAddress, uint16_t sourcePort)
{
    unsigned char key_digest[20];
    memcpy(key_digest, message.GetLookupPublish().lookupDigest, 20);

    if (compareSHA1(m_localDigest,m_successorDigest))
    {
        if (compareSHA1(key_digest,m_localDigest) || compareSHA1 (m_successorDigest,key_digest))
        {
            ReplyLookupPublishSuccess (message,key_digest,sourcePort);
            return;
        }
    }
    else if (compareSHA1(m_successorDigest, key_digest) && compareSHA1(key_digest, m_localDigest))
    {
        ReplyLookupPublishSuccess (message,key_digest,sourcePort);
        return;
    }
    SendLookupPublish (message,key_digest,sourcePort);
}

void
PennChord::SendLookupPublish (PennChordMessage message, unsigned char* digestkey, uint16_t sourcePort)
{
    unsigned char NextHopDigest[20];
    Ipv4Address NextHopAddr = FindNextHop (digestkey);
    SHA_1(NextHopAddr, NextHopDigest);
    CHORD_LOG (" LookupRequest<["<<DisplayHEX(m_localDigest)<<"]: NextHop<"<<NextHopAddr<<", ["<<DisplayHEX(NextHopDigest)<<"], ["<<DisplayHEX(digestkey)<<"]>");
    Ptr<Packet> packet = Create<Packet> ();
    PennChordMessage newMessage = PennChordMessage (PennChordMessage::LOOKUP_PUBLISH, message.GetTransactionId());
    newMessage.SetLookupPublish (message.GetLookupPublish().flag, message.GetLookupPublish().initiatorAddress, digestkey, message.GetLookupPublish().lookupKey);
    packet->AddHeader (newMessage);
    m_socket->SendTo (packet, 0 , InetSocketAddress (NextHopAddr,sourcePort));
    PennChord::globalHopCount++;
}

void
PennChord::ReplyLookupPublishSuccess (PennChordMessage message, unsigned char* targetDigest, uint16_t sourcePort)
{
    Ipv4Address destAddress = message.GetLookupPublish().initiatorAddress;
    CHORD_LOG (" LookupResult<["<<DisplayHEX(m_localDigest)<<"], ["<<DisplayHEX(targetDigest)<<"], "<<ReverseLookup(destAddress)<<">");
    Ptr<Packet> packet = Create<Packet> ();
    PennChordMessage newMessage = PennChordMessage (PennChordMessage::LOOKUP_PUBLISH_SUCCESS,message.GetTransactionId());
    newMessage.SetLookupPublishSuccess (message.GetLookupPublish().flag, m_successorAddr, message.GetLookupPublish().lookupKey);
    packet->AddHeader (newMessage);
    m_socket->SendTo (packet, 0 , InetSocketAddress (destAddress,sourcePort));

}

void
PennChord::ProcessLookupPublishSuccess (PennChordMessage message,Ipv4Address sourceAddress,uint16_t sourcePort)
{
    //CHORD_LOG("Lookup Success reached with result");
    LookupCallback(message.GetLookupPublishSuccess().flag, message.GetLookupPublishSuccess().lookupKey, message.GetLookupPublishSuccess().addressResponsible, message.GetTransactionId());
}

void
PennChord::LookupCallback (uint16_t flag, std::string key, Ipv4Address addressResponsible, uint32_t transactionId)
{
    if (flag==0)
    {
        m_lookupPublishSuccessFn(key, addressResponsible, transactionId);
        return;
    }
    if (flag ==1)
    {
        m_searchInitialSuccessFn(key, addressResponsible, transactionId);
        return;
    }
    if (flag ==2)
    {
        m_searchBeginSuccessFn(key, addressResponsible, transactionId);
        return;
    }
}

///////////////////////////////////////////////////////////

uint32_t
PennChord::GetNextTransactionId ()
{
  return m_currentTransactionId++;
}

void
PennChord::StopChord ()
{
  StopApplication ();
}

void
PennChord::SetPingSuccessCallback (Callback <void, Ipv4Address, std::string> pingSuccessFn)
{
  m_pingSuccessFn = pingSuccessFn;
}


void
PennChord::SetPingFailureCallback (Callback <void, Ipv4Address, std::string> pingFailureFn)
{
  m_pingFailureFn = pingFailureFn;
}

void
PennChord::SetPingRecvCallback (Callback <void, Ipv4Address, std::string> pingRecvFn)
{
  m_pingRecvFn = pingRecvFn;
}

void
PennChord::SetLookupPublishSuccessCallback (Callback <void, std::string, Ipv4Address, uint32_t> lookupPublishSuccessFn)
{
  m_lookupPublishSuccessFn = lookupPublishSuccessFn;
}

void
PennChord::SetSearchInitialSuccessCallback (Callback <void, std::string, Ipv4Address, uint32_t> searchInitialSuccessFn)
{
  m_searchInitialSuccessFn = searchInitialSuccessFn;
}

void
PennChord::SetSearchBeginSuccessCallback (Callback <void, std::string, Ipv4Address, uint32_t> searchBeginSuccessFn)
{
  m_searchBeginSuccessFn = searchBeginSuccessFn;
}

void
PennChord::SetChordJoinNotifyCallback (Callback <void, Ipv4Address, uint32_t> chordJoinNotify)
{
  m_chordJoinNotifyFn = chordJoinNotify;
}

void
PennChord::SetChordLeaveNotifyCallback (Callback <void, Ipv4Address, uint32_t> chordLeaveNotify)
{
  m_chordLeaveNotifyFn = chordLeaveNotify;
}

////////////////////////////////////////////////////////////////////////

void 
PennChord::DoTest (void)
{
  unsigned char digest[20];
  SHA_1(m_localAddress,digest);
  DisplayHEX(digest);
}


void
PennChord::SHA_1 (Ipv4Address ipv4Addr, unsigned char *digest)
{
  std::ostringstream stream;
  stream<<ipv4Addr;
  std::string s=stream.str();  
/*
  unsigned char *ch = new unsigned char [s.size()+1];
  std::copy(s.begin(), s.end(), ch);
  ch[s.size()]='\0';
*/
  //unsigned char digest[20]={0};
 
  SHA1((unsigned char*)s.c_str(),s.size(),digest);
  //SHA1(ch,strlen((const char*)ch),digest);
  //delete[] ch;

  //return digest;
}

void
PennChord::SHA_1 (std::string s, unsigned char *digest)
{
/*
  std::ostringstream stream;
  stream<<ipv4Addr;
  std::string s=stream.str();

  unsigned char *ch = new unsigned char [s.size()+1];
  std::copy(s.begin(), s.end(), ch);
  ch[s.size()]='\0';
*/
  //unsigned char digest[20]={0};

  SHA1((unsigned char*)s.c_str(),s.size(),digest);
  //SHA1(ch,strlen((const char*)ch),digest);
  //delete[] ch;
  //return digest;
}

std::string
PennChord::DisplayHEX (unsigned char *digest)
{
  char const hex[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
  std::string str;
  for (uint8_t i=0; i<20; i++)
  {
    char ch = digest[i];
    str.append(&hex[(ch & 0xF0)>>4],1);
    str.append(&hex[(ch & 0x0F)],1);
  }
  //PRINT_LOG("HEX string" <<"   "<<str);
  return str;
} 

uint8_t
PennChord::compareSHA1 (unsigned char *digest1, unsigned char *digest2)
{
    for (uint8_t i=0; i<20; i++)
    {
        if (digest1[i]>digest2[i])
            return 1;
        else if (digest1[i]<digest2[i])
            return 0;
    }
    return 2;
}

void
PennChord::FindIndexHash (uint16_t index, unsigned char* indexHash)
{
    memcpy(indexHash,m_localDigest, 20);
    uint16_t byteIndex = 20 - ((uint8_t)((index-1)/8) + 1);
    uint16_t bitIndex = (index-1)%8;
    uint16_t byteAdded;
    byteAdded = (1<<bitIndex);

    while(byteIndex >= 0 && byteIndex <=19)
    {
        byteAdded = indexHash[byteIndex] + byteAdded;
        if (byteAdded <=255)
        {
            indexHash[byteIndex] = byteAdded;
            break;
        }
        indexHash[byteIndex] = (byteAdded & 0xFF);
        byteAdded = (byteAdded>>8);
        byteIndex--;
        continue;
    }
    return;
}

