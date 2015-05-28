/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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


#include "ns3/ls-routing-protocol.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/random-variable.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-route.h"
#include "ns3/uinteger.h"
//#include "ns3/test-result.h"
#include <sys/time.h>

using namespace ns3;

#define UINT32_MAX  (0xffffffff)

NS_LOG_COMPONENT_DEFINE ("LSRoutingProtocol");
NS_OBJECT_ENSURE_REGISTERED (LSRoutingProtocol);

TypeId
LSRoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("LSRoutingProtocol")
  .SetParent<PennRoutingProtocol> ()
  .AddConstructor<LSRoutingProtocol> ()
  .AddAttribute ("LSPort",
                 "Listening port for LS packets",
                 UintegerValue (5000),
                 MakeUintegerAccessor (&LSRoutingProtocol::m_lsPort),
                 MakeUintegerChecker<uint16_t> ())
  .AddAttribute ("PingTimeout",
                 "Timeout value for PING_REQ in milliseconds",
                 TimeValue (MilliSeconds (2000)),
                 MakeTimeAccessor (&LSRoutingProtocol::m_pingTimeout),
                  MakeTimeChecker ())
  .AddAttribute ("NdTimeout",
                "Timeout value for ND_REQ in milliseconds",
                TimeValue (MilliSeconds (2000)),
                MakeTimeAccessor (&LSRoutingProtocol::m_ndTimeout),
                 MakeTimeChecker ())
  .AddAttribute ("SingleHop",
                "Single Hop TTL Value",
                UintegerValue (1),
                MakeUintegerAccessor (&LSRoutingProtocol::m_singleHop),
                MakeUintegerChecker<uint8_t> ())
  .AddAttribute ("MaxTTL",
                 "Maximum TTL value for LS packets",
                 UintegerValue (16),
                 MakeUintegerAccessor (&LSRoutingProtocol::m_maxTTL),
                 MakeUintegerChecker<uint8_t> ())
  ;
  return tid;
}

LSRoutingProtocol::LSRoutingProtocol ()
  : m_auditPingsTimer (Timer::CANCEL_ON_DESTROY)
{
  RandomVariable random;
  SeedManager::SetSeed (time (NULL));
  random = UniformVariable (0x00000000, 0xFFFFFFFF);
  m_currentSequenceNumber = random.GetInteger ();
  // Setup static routing 
  m_staticRouting = Create<Ipv4StaticRouting> ();
}

LSRoutingProtocol::~LSRoutingProtocol ()
{
}

void 
LSRoutingProtocol::DoDispose ()
{
  // Close sockets
  for (std::map< Ptr<Socket>, Ipv4InterfaceAddress >::iterator iter = m_socketAddresses.begin ();
       iter != m_socketAddresses.end (); iter++)
    {
      iter->first->Close ();
    }
  m_socketAddresses.clear ();
  
  // Clear static routing
  m_staticRouting = 0;

  // Cancel timers
  m_auditPingsTimer.Cancel ();
 
  m_pingTracker.clear (); 

  PennRoutingProtocol::DoDispose ();
}

void
LSRoutingProtocol::SetMainInterface (uint32_t mainInterface)
{
  m_mainAddress = m_ipv4->GetAddress (mainInterface, 0).GetLocal ();
}

void
LSRoutingProtocol::SetNodeAddressMap (std::map<uint32_t, Ipv4Address> nodeAddressMap)
{
  m_nodeAddressMap = nodeAddressMap;
}

void
LSRoutingProtocol::SetAddressNodeMap (std::map<Ipv4Address, uint32_t> addressNodeMap)
{
  m_addressNodeMap = addressNodeMap;
}

Ipv4Address
LSRoutingProtocol::ResolveNodeIpAddress (uint32_t nodeNumber)
{
  std::map<uint32_t, Ipv4Address>::iterator iter = m_nodeAddressMap.find (nodeNumber);
  if (iter != m_nodeAddressMap.end ())
    { 
      return iter->second;
    }
  return Ipv4Address::GetAny ();
}

std::string
LSRoutingProtocol::ReverseLookup (Ipv4Address ipAddress)
{
  std::map<Ipv4Address, uint32_t>::iterator iter = m_addressNodeMap.find (ipAddress);
  if (iter != m_addressNodeMap.end ())
    { 
      std::ostringstream sin;
      uint32_t nodeNumber = iter->second;
      sin << nodeNumber;    
      return sin.str();
    }
  return "Unknown";
}

void
LSRoutingProtocol::DoStart ()
{
  // Create sockets
  for (uint32_t i = 0 ; i < m_ipv4->GetNInterfaces () ; i++)
    {
      Ipv4Address ipAddress = m_ipv4->GetAddress (i, 0).GetLocal ();
      if (ipAddress == Ipv4Address::GetLoopback ())
        continue;
      // Create socket on this interface
      Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
          UdpSocketFactory::GetTypeId ());
      socket->SetAllowBroadcast (true);
      InetSocketAddress inetAddr (m_ipv4->GetAddress (i, 0).GetLocal (), m_lsPort);
      socket->SetRecvCallback (MakeCallback (&LSRoutingProtocol::RecvLSMessage, this));
      if (socket->Bind (inetAddr))
        {
          NS_FATAL_ERROR ("LSRoutingProtocol::DoStart::Failed to bind socket!");
        }

      //Ipv4Address sourceAddress = inetAddr.GetIpv4 ();
      //std::string Node = ReverseLookup (m_mainAddress);
      //NS_LOG_INFO("Interface address : "<< sourceAddress << " for node :" << Node);

      Ptr<NetDevice> netDevice = m_ipv4->GetNetDevice (i);
      socket->BindToNetDevice (netDevice);
      m_socketAddresses[socket] = m_ipv4->GetAddress (i, 0);
    }
  // Configure timers
  m_auditPingsTimer.SetFunction (&LSRoutingProtocol::AuditPings, this);
  m_ndTimer.SetFunction (&LSRoutingProtocol::NdRequests, this);

  // Start timers
  m_auditPingsTimer.Schedule (m_pingTimeout);
  m_ndTimer.Schedule (m_ndTimeout);
  m_lspSequenceNumber =0;
}

Ptr<Ipv4Route>
LSRoutingProtocol::RouteOutput (Ptr<Packet> packet, const Ipv4Header &header, Ptr<NetDevice> outInterface, Socket::SocketErrno &sockerr)
{ 
  Ptr<Ipv4Route> rEntry;
  uint32_t interfaceNum = m_ipv4->GetInterfaceForDevice (outInterface);
  std::map<uint32_t,RouteTableDetails>::iterator iter;
  iter = m_routeTable.find(m_addressNodeMap.find(header.GetDestination ())->second);
  if(iter != m_routeTable.end())
  {
    rEntry = Create<Ipv4Route>();
    rEntry->SetDestination(header.GetDestination());
    rEntry->SetSource(m_mainAddress);
    rEntry->SetGateway(iter->second.nextHopAddr);
    interfaceNum = m_ipv4->GetInterfaceForAddress(m_neighborTable.find(iter->second.nextHopNumber)->second.interfaceAddr);
    rEntry->SetOutputDevice(m_ipv4->GetNetDevice (interfaceNum));
    TRAFFIC_LOG("Destination: " <<header.GetDestination()<<" interface: "<<interfaceNum);
    sockerr = Socket::ERROR_NOTERROR;
    return rEntry;
  }

  Ptr<Ipv4Route> ipv4Route = m_staticRouting->RouteOutput (packet, header, outInterface, sockerr);
  if (ipv4Route)
    {
      TRAFFIC_LOG ("Found route to: " << ipv4Route->GetDestination () << " via next-hop: " << ipv4Route->GetGateway () << " with source: " << ipv4Route->GetSource () << " and output device " << ipv4Route->GetOutputDevice());
    }
  else
    {
      TRAFFIC_LOG ("No Route to destination: " << header.GetDestination ());
    }
  return ipv4Route;
}

bool 
LSRoutingProtocol::RouteInput  (Ptr<const Packet> packet, 
  const Ipv4Header &header, Ptr<const NetDevice> inputDev,                            
  UnicastForwardCallback ucb, MulticastForwardCallback mcb,             
  LocalDeliverCallback lcb, ErrorCallback ecb)
{
  Ipv4Address destinationAddress = header.GetDestination ();
  Ipv4Address sourceAddress = header.GetSource ();

  // Drop if packet was originated by this node
  if (IsOwnAddress (sourceAddress) == true)
    {
      return true;
    }

  // Check for local delivery
  uint32_t interfaceNum = m_ipv4->GetInterfaceForDevice (inputDev);
  if (m_ipv4->IsDestinationAddress (destinationAddress, interfaceNum))
    {
      if (!lcb.IsNull ())
        {
          lcb (packet, header, interfaceNum);
          return true;
        }
      else
        {
          return false;
        }
    }

  Ptr<Ipv4Route> rEntry;
  std::map<uint32_t,RouteTableDetails>::iterator iter;
  iter = m_routeTable.find(m_addressNodeMap.find(header.GetDestination ())->second);
  if(iter != m_routeTable.end())
  {
    TRAFFIC_LOG("Destination: " <<header.GetDestination());
    rEntry = Create<Ipv4Route>();
    rEntry->SetDestination(header.GetDestination());
    rEntry->SetSource(m_mainAddress);
    rEntry->SetGateway(iter->second.nextHopAddr);
    std::map<uint32_t, NeighborTableEntry>::iterator it = m_neighborTable.find(iter->second.nextHopNumber);
    if (it!=m_neighborTable.end())
    {
      interfaceNum = m_ipv4->GetInterfaceForAddress(it->second.interfaceAddr);
      rEntry->SetOutputDevice(m_ipv4->GetNetDevice (interfaceNum));
      ucb(rEntry, packet, header);
      return true;
    }
  }

  // Check static routing table
  if (m_staticRouting->RouteInput (packet, header, inputDev, ucb, mcb, lcb, ecb))
    {
      return true;
    }
 TRAFFIC_LOG ("Cannot forward packet. No Route to destination: " << header.GetDestination ());
  return false;
}

void
LSRoutingProtocol::BroadcastPacket (Ptr<Packet> packet)
{
  for (std::map<Ptr<Socket> , Ipv4InterfaceAddress>::const_iterator i =
      m_socketAddresses.begin (); i != m_socketAddresses.end (); i++)
    {
      Ipv4Address broadcastAddr = i->second.GetLocal ().GetSubnetDirectedBroadcast (i->second.GetMask ());
      i->first->SendTo (packet, 0, InetSocketAddress (broadcastAddr, m_lsPort));
    }
}

void
LSRoutingProtocol::ProcessCommand (std::vector<std::string> tokens)
{
  std::vector<std::string>::iterator iterator = tokens.begin();
  std::string command = *iterator;
  if (command == "PING")
    {
      if (tokens.size() < 3)
        {
          ERROR_LOG ("Insufficient PING params..."); 
          return;
        }
      iterator++;
      std::istringstream sin (*iterator);
      uint32_t nodeNumber;
      sin >> nodeNumber;
      iterator++;
      std::string pingMessage = *iterator;
      Ipv4Address destAddress = ResolveNodeIpAddress (nodeNumber);
      if (destAddress != Ipv4Address::GetAny ())
        {
          uint32_t sequenceNumber = GetNextSequenceNumber ();
          TRAFFIC_LOG ("Sending PING_REQ to Node: " << nodeNumber << " IP: " << destAddress << " Message: " << pingMessage << " SequenceNumber: " << sequenceNumber);
          Ptr<PingRequest> pingRequest = Create<PingRequest> (sequenceNumber, Simulator::Now(), destAddress, pingMessage);
          // Add to ping-tracker
          m_pingTracker.insert (std::make_pair (sequenceNumber, pingRequest));
          Ptr<Packet> packet = Create<Packet> ();
          LSMessage lsMessage = LSMessage (LSMessage::PING_REQ, sequenceNumber, m_maxTTL, m_mainAddress);
          lsMessage.SetPingReq (destAddress, pingMessage);
          packet->AddHeader (lsMessage);
          BroadcastPacket (packet);
        }
    }
  else if (command == "DUMP")
    {
      if (tokens.size() < 2)
        {
          ERROR_LOG ("Insufficient Parameters!");
          return;
        }
      iterator++;
      std::string table = *iterator;
      if (table == "ROUTES" || table == "ROUTING")
        {
          DumpRoutingTable ();
        }
      else if (table == "NEIGHBORS" || table == "NEIGHBOURS")
        {
          DumpNeighbors ();
        }
      else if (table == "LSA")
        {
          DumpLSA ();
        }
    }
}

void
LSRoutingProtocol::DumpLSA ()
{
  STATUS_LOG (std::endl << "**************** LSA DUMP ********************" << std::endl
              << "Node\t\tNeighbor(s)");
  PRINT_LOG ("");
}

void
LSRoutingProtocol::DumpNeighbors ()
{
  /*STATUS_LOG (std::endl << "**************** Neighbor List ********************" << std::endl
              << "NeighborNumber\t\tNeighborAddr\t\tInterfaceAddr");*/
  PRINT_LOG (std::endl<<m_neighborTable.size());
  for(std::map<uint32_t,NeighborTableEntry>::iterator iter=m_neighborTable.begin();
       iter!=m_neighborTable.end();iter++)
  {
    PRINT_LOG (iter->first <<"\t\t"<< iter->second.neighborAddr <<"\t\t"<<iter->second.interfaceAddr);
  //  checkNeighborTableEntry(iter->first,iter->second.neighborAddr,iter->second.interfaceAddr);
  }

  /*NOTE: For purpose of autograding, you should invoke the following function for each
  neighbor table entry. The output format is indicated by parameter name and type.
  */

}

void
LSRoutingProtocol::DumpRoutingTable ()
{
    /*STATUS_LOG (std::endl << "**************** Route Table ********************" << std::endl
              << "DestNumber\t\tDestAddr\t\tNextHopNumber\t\tNextHopAddr\t\tInterfaceAddr\t\tCost");*/

    PRINT_LOG (m_routeTable.size());
    for(std::map<uint32_t,RouteTableDetails>::iterator iter=m_routeTable.begin();
         iter!=m_routeTable.end();iter++)
    {
      PRINT_LOG (iter->first <<"\t\t"<< iter->second.destAddr <<"\t\t"<<iter->second.nextHopNumber<<
                 "\t\t"<<iter->second.nextHopAddr<<"\t\t"<<iter->second.interfaceAddr<<"\t\t"<<iter->second.cost);
    //  checkRouteTableEntry(iter->first,iter->second.destAddr, iter->second.nextHopNumber, iter->second.nextHopAddr,iter->second.interfaceAddr, iter->second.cost);
    }

	/*NOTE: For purpose of autograding, you should invoke the following function for each
	routing table entry. The output format is indicated by parameter name and type.
	*/

}
void
LSRoutingProtocol::RecvLSMessage (Ptr<Socket> socket)
{
  Address sourceAddr;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddr);
  InetSocketAddress inetSocketAddr = InetSocketAddress::ConvertFrom (sourceAddr);
  Ipv4Address sourceAddress = inetSocketAddr.GetIpv4 ();
  LSMessage lsMessage;
  packet->RemoveHeader (lsMessage);

  switch (lsMessage.GetMessageType ())
    {
      case LSMessage::PING_REQ:
        ProcessPingReq (lsMessage);
        break;
      case LSMessage::PING_RSP:
        ProcessPingRsp (lsMessage);
        break;
      case LSMessage::ND_REQ:
        ProcessNdReq (lsMessage, socket);
        break;
      case LSMessage::ND_RSP:
        ProcessNdRsp (lsMessage, socket);
        break;
     case LSMessage::LSP:
        ProcessLsp (lsMessage, socket);
        break;
      default:
        ERROR_LOG ("Unknown Message Type!");
        break;
    }
}

void
LSRoutingProtocol::ProcessPingReq (LSMessage lsMessage)
{
  // Check destination address
  if (IsOwnAddress (lsMessage.GetPingReq().destinationAddress))
    {
      // Use reverse lookup for ease of debug
      std::string fromNode = ReverseLookup (lsMessage.GetOriginatorAddress ());
      TRAFFIC_LOG ("Received PING_REQ, From Node: " << fromNode << ", Message: " << lsMessage.GetPingReq().pingMessage);
      // Send Ping Response
      LSMessage lsResp = LSMessage (LSMessage::PING_RSP, lsMessage.GetSequenceNumber(), m_maxTTL, m_mainAddress);
      lsResp.SetPingRsp (lsMessage.GetOriginatorAddress(), lsMessage.GetPingReq().pingMessage);
      Ptr<Packet> packet = Create<Packet> ();
      packet->AddHeader (lsResp);
      BroadcastPacket (packet);
    }
}

void
LSRoutingProtocol::ProcessPingRsp (LSMessage lsMessage)
{
  // Check destination address
  if (IsOwnAddress (lsMessage.GetPingRsp().destinationAddress))
    {
      // Remove from pingTracker
      std::map<uint32_t, Ptr<PingRequest> >::iterator iter;
      iter = m_pingTracker.find (lsMessage.GetSequenceNumber ());
      if (iter != m_pingTracker.end ())
        {
          std::string fromNode = ReverseLookup (lsMessage.GetOriginatorAddress ());
          TRAFFIC_LOG ("Received PING_RSP, From Node: " << fromNode << ", Message: " << lsMessage.GetPingRsp().pingMessage);
          m_pingTracker.erase (iter);
        }
      else
        {
          DEBUG_LOG ("Received invalid PING_RSP!");
        }
    }
}

bool
LSRoutingProtocol::IsOwnAddress (Ipv4Address originatorAddress)
{
  // Check all interfaces
  for (std::map<Ptr<Socket> , Ipv4InterfaceAddress>::const_iterator i = m_socketAddresses.begin (); i != m_socketAddresses.end (); i++)
    {
      Ipv4InterfaceAddress interfaceAddr = i->second;
      if (originatorAddress == interfaceAddr.GetLocal ())
        {
          return true;
        }
    }
  return false;

}

void
LSRoutingProtocol::AuditPings ()
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
        }
      else
        {
          ++iter;
        }
    }
  // Rechedule timer
  m_auditPingsTimer.Schedule (m_pingTimeout); 
}

uint32_t
LSRoutingProtocol::GetNextSequenceNumber ()
{
  return m_currentSequenceNumber++;
}

void 
LSRoutingProtocol::NotifyInterfaceUp (uint32_t i)
{
  m_staticRouting->NotifyInterfaceUp (i);
}
void 
LSRoutingProtocol::NotifyInterfaceDown (uint32_t i)
{
  m_staticRouting->NotifyInterfaceDown (i);
}
void 
LSRoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
  m_staticRouting->NotifyAddAddress (interface, address);
}
void 
LSRoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
  m_staticRouting->NotifyRemoveAddress (interface, address);
}

void
LSRoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  m_ipv4 = ipv4;
  m_staticRouting->SetIpv4 (m_ipv4);
}



////////////Neighbour Discovery functions (ND)///////////////////

void
LSRoutingProtocol::NeighborDiscovery ()
{
    //int nodeNumber = 8;
    //Ipv4Address destAddress = ResolveNodeIpAddress (nodeNumber);

    Ipv4Address destAddress; 
    std::string pingMessage = "Neighbour Discovery";

    uint32_t sequenceNumber = GetNextSequenceNumber ();
    //TRAFFIC_LOG ("Sending ND_REQ to Node: random node" << " IP: " << destAddress << " Message: " << pingMessage << " SequenceNumber: " << sequenceNumber);
    Ptr<Packet> packet = Create<Packet> ();
    LSMessage lsMessage = LSMessage (LSMessage::ND_REQ, sequenceNumber, m_singleHop, m_mainAddress);
    lsMessage.SetNdReq (destAddress, pingMessage);
    packet->AddHeader (lsMessage);
    BroadcastPacket (packet);
}

void
LSRoutingProtocol::ProcessNdReq (LSMessage lsMessage, Ptr<Socket> socket)
{
  // Use reverse lookup for ease of debug
  std::string fromNode = ReverseLookup (lsMessage.GetOriginatorAddress ());
  //TRAFFIC_LOG ("Received ND_REQ, From Node: " << fromNode << ", Message: " << lsMessage.GetPingReq().pingMessage);
  // Send ND Response
  LSMessage lsResp = LSMessage (LSMessage::ND_RSP, lsMessage.GetSequenceNumber(), m_singleHop, m_mainAddress);
  lsResp.SetNdRsp (lsMessage.GetOriginatorAddress(), lsMessage.GetPingReq().pingMessage);
  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (lsResp);
  std::map <Ptr<Socket>,Ipv4InterfaceAddress> :: iterator iter;
  iter = m_socketAddresses.find(socket);
  Ipv4Address broadcastAddr = iter->second.GetLocal().GetSubnetDirectedBroadcast (iter->second.GetMask ());
  iter->first->SendTo (packet, 0, InetSocketAddress (broadcastAddr, m_lsPort));
}

void
LSRoutingProtocol::ProcessNdRsp (LSMessage lsMessage,Ptr<Socket> socket)
{

  // Check destination address
  if (IsOwnAddress (lsMessage.GetPingRsp().destinationAddress))
    {
      std::map <Ptr<Socket>,Ipv4InterfaceAddress> :: iterator iter;
      iter = m_socketAddresses.find(socket);
      Ipv4Address InterfaceAddr = iter->second.GetLocal();
      std::map <Ipv4Address, uint32_t>::iterator it = m_addressNodeMap.find(lsMessage.GetOriginatorAddress());
      uint32_t nodeNumber = it->second;
      NeighborTableEntry neighborTableEntry =  (NeighborTableEntry) {lsMessage.GetOriginatorAddress(),InterfaceAddr};
      m_currentneighborTable[nodeNumber]=neighborTableEntry;
      //TRAFFIC_LOG ("Received ND_RSP, From Node: " << nodeNumber << " IPAdress : " << neighborTableEntry.neighborAddr << ", Message: " << lsMessage.GetPingRsp().pingMessage <<" Interface Address : " << neighborTableEntry.interfaceAddr);
    }
}


//Timer to reschedule neighbor discovery
void
LSRoutingProtocol::NdRequests ()
{
    bool floodingFlag=0;
    if (m_neighborTable.size()!=m_currentneighborTable.size())
        floodingFlag=1;
    else
    {
        for(std::map<uint32_t,NeighborTableEntry>::iterator iter1=m_neighborTable.begin(), iter2=m_currentneighborTable.begin();
             iter1!=m_neighborTable.end();)
        {
            if (iter1->first!=iter2->first)
            {
                floodingFlag=1;
                break;
            }
            else if (iter1->second.neighborAddr != iter2->second.neighborAddr || iter1->second.interfaceAddr != iter1->second.interfaceAddr)
            {
                floodingFlag=1;
                break;
            }
            iter1++;
            iter2++;
        }
    }
    m_neighborTable= m_currentneighborTable;
    if (floodingFlag==1)
    {
        Flooding();
        DijkstraAlgo();
    }
    m_currentneighborTable.clear();
    NeighborDiscovery ();
    // Rechedule timer
    m_ndTimer.Schedule (m_ndTimeout);
}

uint64_t
LSRoutingProtocol::GetLspSequenceNumber ()
{
  return m_lspSequenceNumber++;
}

void
LSRoutingProtocol::Flooding ()
{
    Ipv4Address destAddress;
    std::string message = "LSP";
    uint32_t sequenceNumber = GetNextSequenceNumber ();
    uint64_t seqno = GetLspSequenceNumber ();
    NeighborAddrlist.clear();
    for(std::map<uint32_t,NeighborTableEntry>::iterator iter=m_neighborTable.begin();
         iter!=m_neighborTable.end();iter++)
    {
        NeighborAddrlist.push_back(iter->second.neighborAddr);
    }
    //TRAFFIC_LOG ("Sending LSP" << " Message: " << message << " SequenceNumber: " << seqno<< ", TTL set :"<< uint32_t(m_maxTTL));
    Ptr<Packet> packet = Create<Packet> ();
    LSMessage lsMessage = LSMessage (LSMessage::LSP, sequenceNumber, m_maxTTL, m_mainAddress);
    lsMessage.SetLsp (seqno, NeighborAddrlist, destAddress, message);
    packet->AddHeader (lsMessage);
    BroadcastPacket (packet);
}

void
LSRoutingProtocol::ProcessLsp (LSMessage lsMessage,Ptr<Socket> socket)
{
    std::map<Ipv4Address, uint64_t>::iterator i= m_lspSequenceNumberTable.find(lsMessage.GetOriginatorAddress ());
    if (i!=m_lspSequenceNumberTable.end() && lsMessage.GetLsp().sequenceNumber<= i->second)
        return;
    if (i==m_lspSequenceNumberTable.end())
        m_lspSequenceNumberTable[lsMessage.GetOriginatorAddress ()] = lsMessage.GetLsp().sequenceNumber;
    else
        i->second = lsMessage.GetLsp().sequenceNumber;
    std::string fromNode = ReverseLookup (lsMessage.GetOriginatorAddress ());
    std::string message = "Forwarding LSPs";
    //TRAFFIC_LOG ("Received LSP, From Node: " << fromNode<<" with LSP Sequence number: " << lsMessage.GetLsp().sequenceNumber<< " "<< ", Message: " << message << ", TTL Received: " << uint32_t(lsMessage.GetTTL()));
    LSMessage lspForward = LSMessage (LSMessage::LSP, lsMessage.GetSequenceNumber(), lsMessage.GetTTL()-1, lsMessage.GetOriginatorAddress ());
    lspForward.SetLsp (lsMessage.GetLsp().sequenceNumber, lsMessage.GetLsp().NeighborAddrlist,lsMessage.GetLsp().destinationAddress, message);
    Ptr<Packet> packet = Create<Packet> ();
    packet->AddHeader (lspForward);
    ForwardPacket(packet, socket);
    UpdateMap(lsMessage);
}


void
LSRoutingProtocol::ForwardPacket (Ptr<Packet> packet, Ptr<Socket> socket)
{
  for (std::map<Ptr<Socket> , Ipv4InterfaceAddress>::const_iterator i =
      m_socketAddresses.begin (); i != m_socketAddresses.end (); i++)
    {
      if (i->first == socket)
        continue;
      Ipv4Address broadcastAddr = i->second.GetLocal ().GetSubnetDirectedBroadcast (i->second.GetMask ());
      i->first->SendTo (packet, 0, InetSocketAddress (broadcastAddr, m_lsPort));
    }
}


void
LSRoutingProtocol::UpdateMap (LSMessage lsMessage)
{
  uint32_t SourceNode = m_addressNodeMap.find(lsMessage.GetOriginatorAddress())->second;
  RouteMapDetails routeMapDetails;
  routeMapDetails.nodeAddr = lsMessage.GetOriginatorAddress();
  for (uint32_t i=0; i<lsMessage.GetLsp().NeighborAddrlist.size();i++)
  {
    routeMapDetails.neighborListAddr.push_back(lsMessage.GetLsp().NeighborAddrlist[i]);
    routeMapDetails.neighborList.push_back(m_addressNodeMap.find(routeMapDetails.neighborListAddr[i])->second);
    routeMapDetails.neighborListCost.push_back(1);
  }
  m_routeMap[SourceNode]= routeMapDetails;
  DijkstraAlgo();
}

void
LSRoutingProtocol::DumpRouteMap()
{
  for (std::map<uint32_t, RouteMapDetails>::iterator iter= m_routeMap.begin(); 
      iter!=m_routeMap.end();iter++)
  {
    PRINT_LOG (std::endl<<"Neighbors of Node " <<iter->first<<std::endl);
    for (uint32_t i=0; i<iter->second.neighborList.size();i++)
      PRINT_LOG("\t"<<iter->second.neighborList[i]<<"\t"<<iter->second.neighborListAddr[i]);
  }
}  

void
LSRoutingProtocol::DijkstraAlgo ()
{
    std::map<uint32_t, uint32_t> CostMap;
    std::map<uint32_t, uint32_t> NextHopMap;
    //Queue declaration incomplet
    std::priority_queue<std::pair<uint32_t, uint32_t>, std::vector<std::pair<uint32_t, uint32_t> > , CompareCost> Queue;
    for (std::map<uint32_t, RouteMapDetails>::iterator iterCost= m_routeMap.begin();iterCost!=m_routeMap.end();iterCost++)
    {
        CostMap[iterCost->first] = UINT32_MAX;
    }
    for (std::map<uint32_t, NeighborTableEntry>::iterator iter= m_neighborTable.begin(); iter!=m_neighborTable.end();iter++)
    {
        CostMap[iter->first]=1;
        Queue.push(std::make_pair(iter->first, 1));
        NextHopMap[iter->first]=iter->first;
    }
    while(!Queue.empty())
    {
        uint32_t currentNode= Queue.top().first;
        uint32_t currentNeighbor= NextHopMap.find(currentNode)->second;
        Queue.pop();
        std::map<uint32_t, RouteMapDetails>::iterator iter1 = m_routeMap.find(currentNode);
        if (iter1 == m_routeMap.end())
            continue;
        for(uint32_t i=0; i<m_routeMap.find(currentNode)->second.neighborList.size();i++)
        {
            uint32_t connectedNode = iter1->second.neighborList[i];
            std::vector<uint32_t>::iterator it;
            std::map<uint32_t, RouteMapDetails>::iterator iter2= m_routeMap.find(connectedNode);
            if (iter2 == m_routeMap.end())
                continue;
            it=find (iter2->second.neighborList.begin(), iter2->second.neighborList.end(), currentNode);
            if(it==m_routeMap.find(connectedNode)->second.neighborList.end())
               continue;
            uint32_t newCost= CostMap.find(currentNode)->second + m_routeMap.find(currentNode)->second.neighborListCost[i];
            if(CostMap.find(connectedNode)->second > newCost)
            {
                CostMap[connectedNode] = newCost;
                Queue.push(std::make_pair(connectedNode,newCost));
                NextHopMap[connectedNode] = currentNeighbor;
            }

        }
    }
    /*PRINT_LOG(std::endl<<"Cost Map for this node " );
    for (std::map<uint32_t, uint32_t>::iterator iter= CostMap.begin(); iter!=CostMap.end();iter++)
    {
      PRINT_LOG(std::endl<<"\t"<<iter->first<<"\t"<<iter->second);
    }*/
    m_routeTable.clear();
    for (std::map<uint32_t,uint32_t>::iterator iterMap= CostMap.begin();iterMap!=CostMap.end();iterMap++)
    {
        if (iterMap->second == UINT32_MAX)
            continue;
        RouteTableDetails routeTableDetails;
        routeTableDetails.destAddr = m_nodeAddressMap.find(iterMap->first)->second;
        routeTableDetails.nextHopNumber = NextHopMap.find(iterMap->first)->second;
        routeTableDetails.nextHopAddr=  m_nodeAddressMap.find(routeTableDetails.nextHopNumber)->second;
        routeTableDetails.interfaceAddr = m_neighborTable.find(routeTableDetails.nextHopNumber)->second.interfaceAddr;
        routeTableDetails.cost = iterMap->second;
        m_routeTable[iterMap->first]=routeTableDetails;
    }
}

