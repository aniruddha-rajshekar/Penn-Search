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

#include "ns3/penn-chord-message.h"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PennChordMessage");
NS_OBJECT_ENSURE_REGISTERED (PennChordMessage);

PennChordMessage::PennChordMessage ()
{
}

PennChordMessage::~PennChordMessage ()
{
}

PennChordMessage::PennChordMessage (PennChordMessage::MessageType messageType, uint32_t transactionId)
{
  m_messageType = messageType;
  m_transactionId = transactionId;
}

TypeId 
PennChordMessage::GetTypeId (void)
{
  static TypeId tid = TypeId ("PennChordMessage")
    .SetParent<Header> ()
    .AddConstructor<PennChordMessage> ()
  ;
  return tid;
}

TypeId
PennChordMessage::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}


uint32_t
PennChordMessage::GetSerializedSize (void) const
{
  // size of messageType, transaction id
  uint32_t size = sizeof (uint8_t) + sizeof (uint32_t);
  switch (m_messageType)
    {
      case PING_REQ:
        size += m_message.pingReq.GetSerializedSize ();
        break;
      case PING_RSP:
        size += m_message.pingRsp.GetSerializedSize ();
        break;
      case JOIN_CHORD:
        size += m_message.joinChord.GetSerializedSize();
        break;
      case FIND_SUCCESSOR:
        size += m_message.findSuccessor.GetSerializedSize();
        break;
      case JOIN_CHORD_SUCCESS:
        size += m_message.joinChordSuccess.GetSerializedSize();
        break;
      case JOIN_CHORD_FAIL:
         break;
      case NOTIFY:
         break;
      case STABILIZE_REQ:
         break;
      case STABILIZE_RESP:
        size += m_message.stabilizeResp.GetSerializedSize();
        break;
      case RINGSTATE:
        size += m_message.ringstate.GetSerializedSize();
        break;
      case LEAVE_SUCCESSOR:
        size += m_message.leaveSuccessor.GetSerializedSize();
        break;
      case LEAVE_PREDECESSOR:
        size += m_message.leavePredecessor.GetSerializedSize();
        break;
      case FIND_FINGER:
        size += m_message.findFinger.GetSerializedSize();
        break;
      case FIND_FINGER_SUCCESS:
        size += m_message.findFingerSuccess.GetSerializedSize();
        break;
      case LOOKUP_PUBLISH:
        size += m_message.lookupPublish.GetSerializedSize();
        break;
      case LOOKUP_PUBLISH_SUCCESS:
        size += m_message.lookupPublishSuccess.GetSerializedSize();
        break;
      default:
        NS_ASSERT (false);
    }
  return size;
}

void
PennChordMessage::Print (std::ostream &os) const
{
  os << "\n****PennChordMessage Dump****\n" ;
  os << "messageType: " << m_messageType << "\n";
  os << "transactionId: " << m_transactionId << "\n";
  os << "PAYLOAD:: \n";
  
  switch (m_messageType)
    {
      case PING_REQ:
        m_message.pingReq.Print (os);
        break;
      case PING_RSP:
        m_message.pingRsp.Print (os);
        break;
      case JOIN_CHORD:
        m_message.joinChord.Print(os);
        break;
      case FIND_SUCCESSOR:
        m_message.findSuccessor.Print(os);
        break;
      case JOIN_CHORD_SUCCESS:
        m_message.joinChordSuccess.Print(os);
        break;
      case STABILIZE_RESP:
        m_message.stabilizeResp.Print(os);
        break;
      case RINGSTATE:
        m_message.ringstate.Print(os);
        break;
      case LEAVE_SUCCESSOR:
        m_message.leaveSuccessor.Print(os);
        break;
      case LEAVE_PREDECESSOR:
        m_message.leavePredecessor.Print(os);
        break;
      case FIND_FINGER:
        m_message.findFinger.Print(os);
        break;
      case FIND_FINGER_SUCCESS:
        m_message.findFingerSuccess.Print(os);
        break;
      case LOOKUP_PUBLISH:
        m_message.lookupPublish.Print(os);
        break;
      case LOOKUP_PUBLISH_SUCCESS:
        m_message.lookupPublishSuccess.Print(os);
        break;
      default:
        break;  
    }
  os << "\n****END OF MESSAGE****\n";
}

void
PennChordMessage::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_messageType);
  i.WriteHtonU32 (m_transactionId);

  switch (m_messageType)
    {
      case PING_REQ:
        m_message.pingReq.Serialize (i);
        break;
      case PING_RSP:
        m_message.pingRsp.Serialize (i);
        break;
      case JOIN_CHORD:
        m_message.joinChord.Serialize(i);
        break;
      case FIND_SUCCESSOR:
        m_message.findSuccessor.Serialize(i);
        break;
      case JOIN_CHORD_SUCCESS:
        m_message.joinChordSuccess.Serialize(i);
        break;
      case JOIN_CHORD_FAIL:
        break;
      case NOTIFY:
        break;
      case STABILIZE_REQ:
        break;
      case STABILIZE_RESP:
        m_message.stabilizeResp.Serialize(i);
        break;
      case RINGSTATE:
        m_message.ringstate.Serialize(i);
        break;
      case LEAVE_SUCCESSOR:
        m_message.leaveSuccessor.Serialize(i);
        break;
      case LEAVE_PREDECESSOR:
        m_message.leavePredecessor.Serialize(i);
        break;
      case FIND_FINGER:
        m_message.findFinger.Serialize(i);
        break;
      case FIND_FINGER_SUCCESS:
        m_message.findFingerSuccess.Serialize(i);
        break;
      case LOOKUP_PUBLISH:
        m_message.lookupPublish.Serialize(i);
        break;
      case LOOKUP_PUBLISH_SUCCESS:
        m_message.lookupPublishSuccess.Serialize(i);
        break;
      default:
        NS_ASSERT (false);   
    }
}

uint32_t 
PennChordMessage::Deserialize (Buffer::Iterator start)
{
  uint32_t size;
  Buffer::Iterator i = start;
  m_messageType = (MessageType) i.ReadU8 ();
  m_transactionId = i.ReadNtohU32 ();

  size = sizeof (uint8_t) + sizeof (uint32_t);

  switch (m_messageType)
    {
      case PING_REQ:
        size += m_message.pingReq.Deserialize (i);
        break;
      case PING_RSP:
        size += m_message.pingRsp.Deserialize (i);
        break;
      case JOIN_CHORD:
        size += m_message.joinChord.Deserialize(i);
        break;
      case FIND_SUCCESSOR:
        size += m_message.findSuccessor.Deserialize(i);
        break;
      case JOIN_CHORD_SUCCESS:
        size += m_message.joinChordSuccess.Deserialize(i);
        break;
      case JOIN_CHORD_FAIL:
        break;
      case NOTIFY:
        break;
      case STABILIZE_REQ:
        break;
      case STABILIZE_RESP:
        size += m_message.stabilizeResp.Deserialize(i);
        break;
      case RINGSTATE:
        size += m_message.ringstate.Deserialize(i);
        break;
      case LEAVE_SUCCESSOR:
        size += m_message.leaveSuccessor.Deserialize(i);
        break;
      case LEAVE_PREDECESSOR:
        size += m_message.leavePredecessor.Deserialize(i);
        break;
      case FIND_FINGER:
        size += m_message.findFinger.Deserialize(i);
        break;
      case FIND_FINGER_SUCCESS:
        size += m_message.findFingerSuccess.Deserialize(i);
        break;
      case LOOKUP_PUBLISH:
        size += m_message.lookupPublish.Deserialize(i);
        break;
      case LOOKUP_PUBLISH_SUCCESS:
        size += m_message.lookupPublishSuccess.Deserialize(i);
        break;
      default:
        NS_ASSERT (false);
    }
  return size;
}

/* PING_REQ */

uint32_t 
PennChordMessage::PingReq::GetSerializedSize (void) const
{
  uint32_t size;
  size = sizeof(uint16_t) + pingMessage.length();
  return size;
}

void
PennChordMessage::PingReq::Print (std::ostream &os) const
{
  os << "PingReq:: Message: " << pingMessage << "\n";
}

void
PennChordMessage::PingReq::Serialize (Buffer::Iterator &start) const
{
  start.WriteU16 (pingMessage.length ());
  start.Write ((uint8_t *) (const_cast<char*> (pingMessage.c_str())), pingMessage.length());
}

uint32_t
PennChordMessage::PingReq::Deserialize (Buffer::Iterator &start)
{  
  uint16_t length = start.ReadU16 ();
  char* str = (char*) malloc (length);
  start.Read ((uint8_t*)str, length);
  pingMessage = std::string (str, length);
  free (str);
  return PingReq::GetSerializedSize ();
}

void
PennChordMessage::SetPingReq (std::string pingMessage)
{
  if (m_messageType == 0)
    {
      m_messageType = PING_REQ;
    }
  else
    {
      NS_ASSERT (m_messageType == PING_REQ);
    }
  m_message.pingReq.pingMessage = pingMessage;
}

PennChordMessage::PingReq
PennChordMessage::GetPingReq ()
{
  return m_message.pingReq;
}

/* PING_RSP */

uint32_t 
PennChordMessage::PingRsp::GetSerializedSize (void) const
{
  uint32_t size;
  size = sizeof(uint16_t) + pingMessage.length();
  return size;
}

void
PennChordMessage::PingRsp::Print (std::ostream &os) const
{
  os << "PingReq:: Message: " << pingMessage << "\n";
}

void
PennChordMessage::PingRsp::Serialize (Buffer::Iterator &start) const
{
  start.WriteU16 (pingMessage.length ());
  start.Write ((uint8_t *) (const_cast<char*> (pingMessage.c_str())), pingMessage.length());
}

uint32_t
PennChordMessage::PingRsp::Deserialize (Buffer::Iterator &start)
{  
  uint16_t length = start.ReadU16 ();
  char* str = (char*) malloc (length);
  start.Read ((uint8_t*)str, length);
  pingMessage = std::string (str, length);
  free (str);
  return PingRsp::GetSerializedSize ();
}

void
PennChordMessage::SetPingRsp (std::string pingMessage)
{
  if (m_messageType == 0)
    {
      m_messageType = PING_RSP;
    }
  else
    {
      NS_ASSERT (m_messageType == PING_RSP);
    }
  m_message.pingRsp.pingMessage = pingMessage;
}

PennChordMessage::PingRsp
PennChordMessage::GetPingRsp ()
{
  return m_message.pingRsp;
}


/* JOIN_CHORD */

uint32_t
PennChordMessage::JoinChord::GetSerializedSize (void) const
{
  uint32_t size;
  size = 0;
  return size;
}

void
PennChordMessage::JoinChord::Print (std::ostream &os) const
{
  os << "No Payload" << "\n";
}

void
PennChordMessage::JoinChord::Serialize (Buffer::Iterator &start) const
{
  //Nothing to serialize
}

uint32_t
PennChordMessage::JoinChord::Deserialize (Buffer::Iterator &start)
{
  //Nothing to Deserialize
}


/* FIND_SUCCESSOR */

uint32_t 
PennChordMessage::FindSuccessor::GetSerializedSize (void) const
{
  uint32_t size;
  size = IPV4_ADDRESS_SIZE +  20* sizeof(unsigned char);
  return size;
}

void
PennChordMessage::FindSuccessor::Print (std::ostream &os) const
{
  os << "Destination Address : " << destAddress <<"\n";
}

void
PennChordMessage::FindSuccessor::Serialize (Buffer::Iterator &start) const
{
  start.WriteHtonU32 (destAddress.Get());
  start.Write ((uint8_t *) (targetDigest), 20);
}

uint32_t
PennChordMessage::FindSuccessor::Deserialize (Buffer::Iterator &start)
{  
  destAddress = Ipv4Address (start.ReadNtohU32());
  start.Read ((uint8_t*)targetDigest, 20);
  return FindSuccessor::GetSerializedSize ();
}

void
PennChordMessage::SetFindSuccessor (Ipv4Address destAddr, unsigned char *m_digest)
{
  if (m_messageType == 0)
    {
      m_messageType = FIND_SUCCESSOR;
    }
  else
    {
      NS_ASSERT (m_messageType = FIND_SUCCESSOR);
    }
  m_message.findSuccessor.destAddress = destAddr;
  memcpy (m_message.findSuccessor.targetDigest, m_digest,20);

}

PennChordMessage::FindSuccessor
PennChordMessage::GetFindSuccessor ()
{
  return m_message.findSuccessor;
}

/* JOIN_CHORD_SUCCESS */

uint32_t 
PennChordMessage::JoinChordSuccess::GetSerializedSize (void) const
{
  uint32_t size;
  size = IPV4_ADDRESS_SIZE ;
  return size;
}

void
PennChordMessage::JoinChordSuccess::Print (std::ostream &os) const
{
  os << "Successor Address : " << successorAddress << "\n";
}

void
PennChordMessage::JoinChordSuccess::Serialize (Buffer::Iterator &start) const
{
  start.WriteHtonU32 (successorAddress.Get());
}

uint32_t
PennChordMessage::JoinChordSuccess::Deserialize (Buffer::Iterator &start)
{  
  successorAddress = Ipv4Address (start.ReadNtohU32());
  return JoinChordSuccess::GetSerializedSize ();
}

void
PennChordMessage::SetJoinChordSuccess (Ipv4Address successorAddr)
{
  if (m_messageType == 0)
    {
      m_messageType = JOIN_CHORD_SUCCESS;
    }
  else
    {
      NS_ASSERT (m_messageType = JOIN_CHORD_SUCCESS);
    }
  m_message.joinChordSuccess.successorAddress = successorAddr;
}

PennChordMessage::JoinChordSuccess
PennChordMessage::GetJoinChordSuccess ()
{
  return m_message.joinChordSuccess;
}

/* STABILIZE_RESP */

uint32_t
PennChordMessage::StabilizeResp::GetSerializedSize (void) const
{
  uint32_t size;
  size = IPV4_ADDRESS_SIZE ;
  return size;
}

void
PennChordMessage::StabilizeResp::Print (std::ostream &os) const
{
  os << "Predecessor Address : " << predecessorAddress << "\n";
}

void
PennChordMessage::StabilizeResp::Serialize (Buffer::Iterator &start) const
{
  start.WriteHtonU32 (predecessorAddress.Get());
}

uint32_t
PennChordMessage::StabilizeResp::Deserialize (Buffer::Iterator &start)
{
  predecessorAddress = Ipv4Address (start.ReadNtohU32());
  return StabilizeResp::GetSerializedSize ();
}

void
PennChordMessage::SetStabilizeResp (Ipv4Address predecessorAddr)
{
  if (m_messageType == 0)
    {
      m_messageType = STABILIZE_RESP;
    }
  else
    {
      NS_ASSERT (m_messageType = STABILIZE_RESP);
    }
  m_message.stabilizeResp.predecessorAddress = predecessorAddr;
}

PennChordMessage::StabilizeResp
PennChordMessage::GetStabilizeResp ()
{
  return m_message.stabilizeResp;
}

/*RINGSTATE*/

uint32_t
PennChordMessage::Ringstate::GetSerializedSize (void) const
{
  uint32_t size;
  size = IPV4_ADDRESS_SIZE ;
  return size;
}

void
PennChordMessage::Ringstate::Print (std::ostream &os) const
{
  os << "Initiator Address : " << initiatorAddress << "\n";
}

void
PennChordMessage::Ringstate::Serialize (Buffer::Iterator &start) const
{
  start.WriteHtonU32 (initiatorAddress.Get());
}

uint32_t
PennChordMessage::Ringstate::Deserialize (Buffer::Iterator &start)
{
  initiatorAddress = Ipv4Address (start.ReadNtohU32());
  return Ringstate::GetSerializedSize ();
}

void
PennChordMessage::SetRingstate (Ipv4Address initiatorAddr)
{
  if (m_messageType == 0)
    {
      m_messageType = RINGSTATE;
    }
  else
    {
      NS_ASSERT (m_messageType = RINGSTATE);
    }
  m_message.ringstate.initiatorAddress = initiatorAddr;
}

PennChordMessage::Ringstate
PennChordMessage::GetRingstate ()
{
  return m_message.ringstate;
}

/*LEAVE_SUCCESSOR*/

uint32_t
PennChordMessage::LeaveSuccessor::GetSerializedSize (void) const
{
  uint32_t size;
  size = IPV4_ADDRESS_SIZE ;
  return size;
}

void
PennChordMessage::LeaveSuccessor::Print (std::ostream &os) const
{
  os << "Predecessor Address : " << predecessorAddress << "\n";
}

void
PennChordMessage::LeaveSuccessor::Serialize (Buffer::Iterator &start) const
{
  start.WriteHtonU32 (predecessorAddress.Get());
}

uint32_t
PennChordMessage::LeaveSuccessor::Deserialize (Buffer::Iterator &start)
{
  predecessorAddress = Ipv4Address (start.ReadNtohU32());
  return LeaveSuccessor::GetSerializedSize ();
}

void
PennChordMessage::SetLeaveSuccessor (Ipv4Address predecessorAddr)
{
  if (m_messageType == 0)
    {
      m_messageType = LEAVE_SUCCESSOR;
    }
  else
    {
      NS_ASSERT (m_messageType = LEAVE_SUCCESSOR);
    }
  m_message.leaveSuccessor.predecessorAddress = predecessorAddr;
}

PennChordMessage::LeaveSuccessor
PennChordMessage::GetLeaveSuccessor ()
{
  return m_message.leaveSuccessor;
}

/*LEAVE_PREDECESSOR*/

uint32_t
PennChordMessage::LeavePredecessor::GetSerializedSize (void) const
{
  uint32_t size;
  size = IPV4_ADDRESS_SIZE ;
  return size;
}

void
PennChordMessage::LeavePredecessor::Print (std::ostream &os) const
{
  os << "Successor Address : " << successorAddress << "\n";
}

void
PennChordMessage::LeavePredecessor::Serialize (Buffer::Iterator &start) const
{
  start.WriteHtonU32 (successorAddress.Get());
}

uint32_t
PennChordMessage::LeavePredecessor::Deserialize (Buffer::Iterator &start)
{
  successorAddress = Ipv4Address (start.ReadNtohU32());
  return LeavePredecessor::GetSerializedSize ();
}

void
PennChordMessage::SetLeavePredecessor (Ipv4Address successorAddr)
{
  if (m_messageType == 0)
    {
      m_messageType = LEAVE_PREDECESSOR;
    }
  else
    {
      NS_ASSERT (m_messageType = LEAVE_PREDECESSOR);
    }
  m_message.leavePredecessor.successorAddress = successorAddr;
}

PennChordMessage::LeavePredecessor
PennChordMessage::GetLeavePredecessor ()
{
  return m_message.leavePredecessor;
}

/*FIND_FINGER*/

uint32_t
PennChordMessage::FindFinger::GetSerializedSize (void) const
{
  uint32_t size;
  size = IPV4_ADDRESS_SIZE +  20* sizeof(unsigned char) + sizeof(uint16_t);
  return size;
}

void
PennChordMessage::FindFinger::Print (std::ostream &os) const
{
  os << "Target Index : " << targetIndex << "\n";
}

void
PennChordMessage::FindFinger::Serialize (Buffer::Iterator &start) const
{
  start.WriteHtonU32 (targetAddress.Get());
  start.Write ((uint8_t *) (targetDigest), 20);
  start.WriteU16 (targetIndex);
}

uint32_t
PennChordMessage::FindFinger::Deserialize (Buffer::Iterator &start)
{
    targetAddress = Ipv4Address (start.ReadNtohU32());
    start.Read ((uint8_t*)targetDigest, 20);
    targetIndex = start.ReadU16();
    return FindFinger::GetSerializedSize ();
}

void
PennChordMessage::SetFindFinger (Ipv4Address targetAddr, unsigned char *targetDig, uint16_t targetInd)
{
  if (m_messageType == 0)
    {
      m_messageType = FIND_FINGER;
    }
  else
    {
      NS_ASSERT (m_messageType = FIND_FINGER);
    }
  m_message.findFinger.targetAddress = targetAddr;
  memcpy (m_message.findFinger.targetDigest, targetDig,20);
  m_message.findFinger.targetIndex = targetInd;
}

PennChordMessage::FindFinger
PennChordMessage::GetFindFinger ()
{
  return m_message.findFinger;
}

/*FIND_FINGER_SUCCESS*/

uint32_t
PennChordMessage::FindFingerSuccess::GetSerializedSize (void) const
{
  uint32_t size;
  size = IPV4_ADDRESS_SIZE +  sizeof(uint16_t);
  return size;
}

void
PennChordMessage::FindFingerSuccess::Print (std::ostream &os) const
{
  os << "Finger Index : " << fingerIndex << " Finger Address : "<< fingerAddress << "\n";
}

void
PennChordMessage::FindFingerSuccess::Serialize (Buffer::Iterator &start) const
{
  start.WriteHtonU32 (fingerAddress.Get());
  start.WriteU16 (fingerIndex);
}

uint32_t
PennChordMessage::FindFingerSuccess::Deserialize (Buffer::Iterator &start)
{
    fingerAddress = Ipv4Address (start.ReadNtohU32());
    fingerIndex = start.ReadU16();
    return FindFingerSuccess::GetSerializedSize ();
}

void
PennChordMessage::SetFindFingerSuccess (Ipv4Address fingerAddr, uint16_t fingerInd)
{
  if (m_messageType == 0)
    {
      m_messageType = FIND_FINGER_SUCCESS;
    }
  else
    {
      NS_ASSERT (m_messageType = FIND_FINGER_SUCCESS);
    }
  m_message.findFingerSuccess.fingerAddress = fingerAddr;
  m_message.findFingerSuccess.fingerIndex = fingerInd;
}

PennChordMessage::FindFingerSuccess
PennChordMessage::GetFindFingerSuccess ()
{
  return m_message.findFingerSuccess;
}

/*LOOKUP_PUBLISH*/

uint32_t
PennChordMessage::LookupPublish::GetSerializedSize (void) const
{
  uint32_t size;
  size = sizeof(uint16_t) + IPV4_ADDRESS_SIZE +  20* sizeof(unsigned char) + sizeof(uint16_t) + lookupKey.length();
  return size;
}

void
PennChordMessage::LookupPublish::Print (std::ostream &os) const
{
  os << "Lookup Key : " << lookupKey << "\n";
}

void
PennChordMessage::LookupPublish::Serialize (Buffer::Iterator &start) const
{
  start.WriteU16 (flag);
  start.WriteHtonU32 (initiatorAddress.Get());
  start.Write ((uint8_t *) (lookupDigest), 20);
  start.WriteU16 (lookupKey.length ());
  start.Write ((uint8_t *) (const_cast<char*> (lookupKey.c_str())), lookupKey.length());
}

uint32_t
PennChordMessage::LookupPublish::Deserialize (Buffer::Iterator &start)
{
    flag = start.ReadU16();
    initiatorAddress = Ipv4Address (start.ReadNtohU32());
    start.Read ((uint8_t*)lookupDigest, 20);
    uint16_t length = start.ReadU16 ();
    char* str = (char*) malloc (length);
    start.Read ((uint8_t*)str, length);
    lookupKey = std::string (str, length);
    free (str);
    return LookupPublish::GetSerializedSize ();
}

void
PennChordMessage::SetLookupPublish (uint16_t flag, Ipv4Address initiatorAddr, unsigned char *lookupDig, std::string lookupKey)
{
  if (m_messageType == 0)
    {
      m_messageType = LOOKUP_PUBLISH;
    }
  else
    {
      NS_ASSERT (m_messageType = LOOKUP_PUBLISH);
    }
  m_message.lookupPublish.flag = flag;
  m_message.lookupPublish.initiatorAddress = initiatorAddr;
  memcpy (m_message.lookupPublish.lookupDigest, lookupDig,20);
  m_message.lookupPublish.lookupKey = lookupKey;
}

PennChordMessage::LookupPublish
PennChordMessage::GetLookupPublish ()
{
  return m_message.lookupPublish;
}

/*LOOKUP_PUBLISH_SUCCESS*/

uint32_t
PennChordMessage::LookupPublishSuccess::GetSerializedSize (void) const
{
  uint32_t size;
  size = sizeof(uint16_t) +IPV4_ADDRESS_SIZE + sizeof(uint16_t) + lookupKey.length();
  return size;
}

void
PennChordMessage::LookupPublishSuccess::Print (std::ostream &os) const
{
  os << "Lookup Key : " << lookupKey << "\n";
}

void
PennChordMessage::LookupPublishSuccess::Serialize (Buffer::Iterator &start) const
{
  start.WriteU16 (flag);
  start.WriteHtonU32 (addressResponsible.Get());
  start.WriteU16 (lookupKey.length ());
  start.Write ((uint8_t *) (const_cast<char*> (lookupKey.c_str())), lookupKey.length());
}

uint32_t
PennChordMessage::LookupPublishSuccess::Deserialize (Buffer::Iterator &start)
{
    flag = start.ReadU16();
    addressResponsible = Ipv4Address (start.ReadNtohU32());
    uint16_t length = start.ReadU16 ();
    char* str = (char*) malloc (length);
    start.Read ((uint8_t*)str, length);
    lookupKey = std::string (str, length);
    free (str);
    return LookupPublishSuccess::GetSerializedSize ();
}

void
PennChordMessage::SetLookupPublishSuccess (uint16_t flag, Ipv4Address addressResp, std::string lookupKey)
{
  if (m_messageType == 0)
    {
      m_messageType = LOOKUP_PUBLISH_SUCCESS;
    }
  else
    {
      NS_ASSERT (m_messageType = LOOKUP_PUBLISH_SUCCESS);
    }
  m_message.lookupPublishSuccess.flag = flag;
  m_message.lookupPublishSuccess.addressResponsible = addressResp;
  m_message.lookupPublishSuccess.lookupKey = lookupKey;
}

PennChordMessage::LookupPublishSuccess
PennChordMessage::GetLookupPublishSuccess ()
{
  return m_message.lookupPublishSuccess;
}




//
//
//

void
PennChordMessage::SetMessageType (MessageType messageType)
{
  m_messageType = messageType;
}

PennChordMessage::MessageType
PennChordMessage::GetMessageType () const
{
  return m_messageType;
}

void
PennChordMessage::SetTransactionId (uint32_t transactionId)
{
  m_transactionId = transactionId;
}

uint32_t 
PennChordMessage::GetTransactionId (void) const
{
  return m_transactionId;
}

