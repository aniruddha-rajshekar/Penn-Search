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

#include "ns3/penn-search-message.h"
#include "ns3/log.h"

#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PennSearchMessage");
NS_OBJECT_ENSURE_REGISTERED (PennSearchMessage);

PennSearchMessage::PennSearchMessage ()
{
}

PennSearchMessage::~PennSearchMessage ()
{
}

PennSearchMessage::PennSearchMessage (PennSearchMessage::MessageType messageType, uint32_t transactionId)
{
  m_messageType = messageType;
  m_transactionId = transactionId;
}

TypeId 
PennSearchMessage::GetTypeId (void)
{
  static TypeId tid = TypeId ("PennSearchMessage")
    .SetParent<Header> ()
    .AddConstructor<PennSearchMessage> ()
  ;
  return tid;
}

TypeId
PennSearchMessage::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}


uint32_t
PennSearchMessage::GetSerializedSize (void) const
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
      case STORE_LIST:
        size += m_message.storeList.GetSerializedSize ();
        break;
      case SEARCH_INITIAL:
        size += m_message.searchInitial.GetSerializedSize ();
        break;
      case SEARCH_BEGIN:
        size += m_message.searchBegin.GetSerializedSize ();
        break;
      case SEARCH:
        size += m_message.search.GetSerializedSize ();
        break;
      case SEARCH_COMPLETE:
        size += m_message.searchComplete.GetSerializedSize();
        break;
      case PASS_KEYS:
        size += m_message.passKeys.GetSerializedSize();
        break;
      default:
        NS_ASSERT (false);
    }
  return size;
}

void
PennSearchMessage::Print (std::ostream &os) const
{
  os << "\n****PennSearchMessage Dump****\n" ;
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
      case STORE_LIST:
        m_message.storeList.Print (os);
        break;
      case SEARCH_INITIAL:
        m_message.searchInitial.Print (os);
        break;
      case SEARCH_BEGIN:
        m_message.searchBegin.Print (os);
        break;
      case SEARCH:
        m_message.search.Print (os);
        break;
      case SEARCH_COMPLETE:
        m_message.searchComplete.Print(os);
        break;
      case PASS_KEYS:
        m_message.passKeys.Print(os);
        break;
      default:
        break;  
    }
  os << "\n****END OF MESSAGE****\n";
}

void
PennSearchMessage::Serialize (Buffer::Iterator start) const
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
      case STORE_LIST:
        m_message.storeList.Serialize (i);
        break;
      case SEARCH_INITIAL:
        m_message.searchInitial.Serialize (i);
        break;
      case SEARCH_BEGIN:
        m_message.searchBegin.Serialize (i);
        break;
      case SEARCH:
        m_message.search.Serialize (i);
        break;
      case SEARCH_COMPLETE:
        m_message.searchComplete.Serialize(i);
        break;
      case PASS_KEYS:
        m_message.passKeys.Serialize(i);
        break;
      default:
        NS_ASSERT (false);   
    }
}

uint32_t 
PennSearchMessage::Deserialize (Buffer::Iterator start)
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
      case STORE_LIST:
        size += m_message.storeList.Deserialize (i);
        break;
      case SEARCH_INITIAL:
        size += m_message.searchInitial.Deserialize (i);
        break;
      case SEARCH_BEGIN:
        size += m_message.searchBegin.Deserialize (i);
        break;
      case SEARCH:
        size += m_message.search.Deserialize (i);
        break;
      case SEARCH_COMPLETE:
        size += m_message.searchComplete.Deserialize(i);
        break;
      case PASS_KEYS:
        size += m_message.passKeys.Deserialize(i);
        break;
      default:
        NS_ASSERT (false);
    }
  return size;
}

/* PING_REQ */

uint32_t
PennSearchMessage::PingReq::GetSerializedSize (void) const
{
  uint32_t size;
  size = sizeof(uint16_t) + pingMessage.length();
  return size;
}

void
PennSearchMessage::PingReq::Print (std::ostream &os) const
{
  os << "PingReq:: Message: " << pingMessage << "\n";
}

void
PennSearchMessage::PingReq::Serialize (Buffer::Iterator &start) const
{
  start.WriteU16 (pingMessage.length ());
  start.Write ((uint8_t *) (const_cast<char*> (pingMessage.c_str())), pingMessage.length());
}

uint32_t
PennSearchMessage::PingReq::Deserialize (Buffer::Iterator &start)
{
  uint16_t length = start.ReadU16 ();
  char* str = (char*) malloc (length);
  start.Read ((uint8_t*)str, length);
  pingMessage = std::string (str, length);
  free (str);
  return PingReq::GetSerializedSize ();
}

void
PennSearchMessage::SetPingReq (std::string pingMessage)
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

PennSearchMessage::PingReq
PennSearchMessage::GetPingReq ()
{
  return m_message.pingReq;
}

/* PING_RSP */

uint32_t 
PennSearchMessage::PingRsp::GetSerializedSize (void) const
{
  uint32_t size;
  size = sizeof(uint16_t) + pingMessage.length();
  return size;
}

void
PennSearchMessage::PingRsp::Print (std::ostream &os) const
{
  os << "PingReq:: Message: " << pingMessage << "\n";
}

void
PennSearchMessage::PingRsp::Serialize (Buffer::Iterator &start) const
{
  start.WriteU16 (pingMessage.length ());
  start.Write ((uint8_t *) (const_cast<char*> (pingMessage.c_str())), pingMessage.length());
}

uint32_t
PennSearchMessage::PingRsp::Deserialize (Buffer::Iterator &start)
{  
  uint16_t length = start.ReadU16 ();
  char* str = (char*) malloc (length);
  start.Read ((uint8_t*)str, length);
  pingMessage = std::string (str, length);
  free (str);
  return PingRsp::GetSerializedSize ();
}

void
PennSearchMessage::SetPingRsp (std::string pingMessage)
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

PennSearchMessage::PingRsp
PennSearchMessage::GetPingRsp ()
{
  return m_message.pingRsp;
}

/* STORE_LIST */

uint32_t
PennSearchMessage::StoreList::GetSerializedSize (void) const
{
  uint32_t size;
  size = sizeof(uint16_t) + key.length() + sizeof(uint16_t);
  for(uint16_t i=0;i<docVector.size();i++)
  {
      size+= sizeof(uint16_t) + docVector[i].length();
  }
  return size;
}

void
PennSearchMessage::StoreList::Print (std::ostream &os) const
{
  os << "Store List Key" << key << "\n";
}

void
PennSearchMessage::StoreList::Serialize (Buffer::Iterator &start) const
{
  start.WriteU16 (key.length ());
  start.Write ((uint8_t *) (const_cast<char*> (key.c_str())), key.length());
  start.WriteU16 (docVector.size());
  for(uint16_t i=0;i<docVector.size();i++)
  {
      start.WriteU16 (docVector[i].length ());
      start.Write ((uint8_t *) (const_cast<char*> (docVector[i].c_str())), docVector[i].length());
  }
}

uint32_t
PennSearchMessage::StoreList::Deserialize (Buffer::Iterator &start)
{
  uint16_t length = start.ReadU16 ();
  char* str = (char*) malloc (length);
  start.Read ((uint8_t*)str, length);
  key = std::string (str, length);
  free (str);

  uint16_t vectorSize = start.ReadU16 ();
  for(uint16_t i=0; i<vectorSize; i++)
  {
      length = start.ReadU16 ();
      char* str = (char*) malloc (length);
      start.Read ((uint8_t*)str, length);
      docVector.push_back(std::string (str, length));
      free (str);
  }
  return StoreList::GetSerializedSize ();
}

void
PennSearchMessage::SetStoreList (std::string key, std::vector<std::string> docVector)
{
  if (m_messageType == 0)
    {
      m_messageType = STORE_LIST;
    }
  else
    {
      NS_ASSERT (m_messageType == STORE_LIST);
    }
  m_message.storeList.key = key;
  m_message.storeList.docVector = docVector;
}

PennSearchMessage::StoreList
PennSearchMessage::GetStoreList ()
{
  return m_message.storeList;
}

/* SEARCH_INITIAL */

uint32_t
PennSearchMessage::SearchInitial::GetSerializedSize (void) const
{
  uint32_t size;
  size = IPV4_ADDRESS_SIZE + sizeof(uint16_t);
  for(uint16_t i=0;i<keyList.size();i++)
  {
      size+= sizeof(uint16_t) + keyList[i].length();
  }
  return size;
}

void
PennSearchMessage::SearchInitial::Print (std::ostream &os) const
{
    os << "Initiator Address" << initiatorAddress << "\n";
}

void
PennSearchMessage::SearchInitial::Serialize (Buffer::Iterator &start) const
{
  start.WriteHtonU32 (initiatorAddress.Get());
  start.WriteU16 (keyList.size());
  for(uint16_t i=0;i<keyList.size();i++)
  {
      start.WriteU16 (keyList[i].length ());
      start.Write ((uint8_t *) (const_cast<char*> (keyList[i].c_str())), keyList[i].length());
  }
}

uint32_t
PennSearchMessage::SearchInitial::Deserialize (Buffer::Iterator &start)
{
  initiatorAddress = Ipv4Address (start.ReadNtohU32());
  uint16_t length;
  uint16_t vectorSize = start.ReadU16 ();
  for(uint16_t i=0; i<vectorSize; i++)
  {
      length = start.ReadU16 ();
      char* str = (char*) malloc (length);
      start.Read ((uint8_t*)str, length);
      keyList.push_back(std::string (str, length));
      free (str);
  }
  return SearchInitial::GetSerializedSize ();
}

void
PennSearchMessage::SetSearchInitial (Ipv4Address initiatorAddress, std::vector<std::string> keyList)
{
  if (m_messageType == 0)
    {
      m_messageType = SEARCH_INITIAL;
    }
  else
    {
      NS_ASSERT (m_messageType == SEARCH_INITIAL);
    }
  m_message.searchInitial.initiatorAddress = initiatorAddress;
  m_message.searchInitial.keyList = keyList;
}

PennSearchMessage::SearchInitial
PennSearchMessage::GetSearchInitial ()
{
  return m_message.searchInitial;
}

/* SEARCH_BEGIN */

uint32_t
PennSearchMessage::SearchBegin::GetSerializedSize (void) const
{
  uint32_t size;
  size = IPV4_ADDRESS_SIZE + sizeof(uint16_t) + sizeof(uint16_t);
  for(uint16_t i=0;i<keyList.size();i++)
  {
      size+= sizeof(uint16_t) + keyList[i].length();
  }
  for(uint16_t i=0;i<docList.size();i++)
  {
      size+= sizeof(uint16_t) + docList[i].length();
  }
  return size;
}

void
PennSearchMessage::SearchBegin::Print (std::ostream &os) const
{
  os << "Initiator Address" << initiatorAddress << "\n";
}

void
PennSearchMessage::SearchBegin::Serialize (Buffer::Iterator &start) const
{
  start.WriteHtonU32 (initiatorAddress.Get());
  uint16_t length;
  start.WriteU16 (keyList.size());
  for(uint16_t i=0;i<keyList.size();i++)
  {
      start.WriteU16 (keyList[i].length ());
      start.Write ((uint8_t *) (const_cast<char*> (keyList[i].c_str())), keyList[i].length());
  }
  start.WriteU16 (docList.size());
  for(uint16_t i=0;i<docList.size();i++)
  {
      start.WriteU16 (docList[i].length ());
      start.Write ((uint8_t *) (const_cast<char*> (docList[i].c_str())), docList[i].length());
  }
}

uint32_t
PennSearchMessage::SearchBegin::Deserialize (Buffer::Iterator &start)
{
  initiatorAddress = Ipv4Address (start.ReadNtohU32());
   uint16_t length;
  uint16_t vectorSize = start.ReadU16 ();
  for(uint16_t i=0; i<vectorSize; i++)
  {
      length = start.ReadU16 ();
      char* str = (char*) malloc (length);
      start.Read ((uint8_t*)str, length);
      keyList.push_back(std::string (str, length));
      free (str);
  }

  vectorSize = start.ReadU16 ();
  for(uint16_t i=0; i<vectorSize; i++)
  {
      length = start.ReadU16 ();
      char* str = (char*) malloc (length);
      start.Read ((uint8_t*)str, length);
      docList.push_back(std::string (str, length));
      free (str);
  }
  return SearchBegin::GetSerializedSize ();
}

void
PennSearchMessage::SetSearchBegin (Ipv4Address initiatorAddress, std::vector<std::string> keyList, std::vector<std::string> docList)
{
  if (m_messageType == 0)
    {
      m_messageType = SEARCH_BEGIN;
    }
  else
    {
      NS_ASSERT (m_messageType == SEARCH_BEGIN);
    }
  m_message.searchBegin.initiatorAddress = initiatorAddress;
  m_message.searchBegin.keyList = keyList;
  m_message.searchBegin.docList = docList;
}

PennSearchMessage::SearchBegin
PennSearchMessage::GetSearchBegin ()
{
  return m_message.searchBegin;
}

/* SEARCH */

uint32_t
PennSearchMessage::Search::GetSerializedSize (void) const
{
  uint32_t size;
  size = IPV4_ADDRESS_SIZE + sizeof(uint16_t) + sizeof(uint16_t);
  for(uint16_t i=0;i<keyList.size();i++)
  {
      size+= sizeof(uint16_t) + keyList[i].length();
  }
  for(uint16_t i=0;i<docList.size();i++)
  {
      size+= sizeof(uint16_t) + docList[i].length();
  }
  return size;
}

void
PennSearchMessage::Search::Print (std::ostream &os) const
{
  os << "Initiator Address" << initiatorAddress << "\n";
}

void
PennSearchMessage::Search::Serialize (Buffer::Iterator &start) const
{
  start.WriteHtonU32 (initiatorAddress.Get());
  uint16_t length;
  start.WriteU16 (keyList.size());
  for(uint16_t i=0;i<keyList.size();i++)
  {
      start.WriteU16 (keyList[i].length ());
      start.Write ((uint8_t *) (const_cast<char*> (keyList[i].c_str())), keyList[i].length());
  }
  start.WriteU16 (docList.size());
  for(uint16_t i=0;i<docList.size();i++)
  {
      start.WriteU16 (docList[i].length ());
      start.Write ((uint8_t *) (const_cast<char*> (docList[i].c_str())), docList[i].length());
  }
}

uint32_t
PennSearchMessage::Search::Deserialize (Buffer::Iterator &start)
{
  initiatorAddress = Ipv4Address (start.ReadNtohU32());
   uint16_t length;
  uint16_t vectorSize = start.ReadU16 ();
  for(uint16_t i=0; i<vectorSize; i++)
  {
      length = start.ReadU16 ();
      char* str = (char*) malloc (length);
      start.Read ((uint8_t*)str, length);
      keyList.push_back(std::string (str, length));
      free (str);
  }

  vectorSize = start.ReadU16 ();
  for(uint16_t i=0; i<vectorSize; i++)
  {
      length = start.ReadU16 ();
      char* str = (char*) malloc (length);
      start.Read ((uint8_t*)str, length);
      docList.push_back(std::string (str, length));
      free (str);
  }
  return Search::GetSerializedSize ();
}

void
PennSearchMessage::SetSearch (Ipv4Address initiatorAddress, std::vector<std::string> keyList, std::vector<std::string> docList)
{
  if (m_messageType == 0)
    {
      m_messageType = SEARCH;
    }
  else
    {
      NS_ASSERT (m_messageType == SEARCH);
    }
  m_message.search.initiatorAddress = initiatorAddress;
  m_message.search.keyList = keyList;
  m_message.search.docList = docList;
}

PennSearchMessage::Search
PennSearchMessage::GetSearch ()
{
  return m_message.search;
}

/* SEARCH_COMPLETE */

uint32_t
PennSearchMessage::SearchComplete::GetSerializedSize (void) const
{
  uint32_t size;
  size = sizeof(uint16_t) + sizeof(uint16_t);
  for(uint16_t i=0;i<keyList.size();i++)
  {
      size+= sizeof(uint16_t) + keyList[i].length();
  }
  for(uint16_t i=0;i<docList.size();i++)
  {
      size+= sizeof(uint16_t) + docList[i].length();
  }
  return size;
}

void
PennSearchMessage::SearchComplete::Print (std::ostream &os) const
{
    for(uint16_t i=0;i<docList.size();i++)
    {
        os << "Docs "<<i<<".  "<< docList[i] << "\n";
    }
}

void
PennSearchMessage::SearchComplete::Serialize (Buffer::Iterator &start) const
{
  start.WriteU16 (keyList.size());
  for(uint16_t i=0;i<keyList.size();i++)
  {
      start.WriteU16 (keyList[i].length ());
      start.Write ((uint8_t *) (const_cast<char*> (keyList[i].c_str())), keyList[i].length());
  }
  start.WriteU16 (docList.size());
  for(uint16_t i=0;i<docList.size();i++)
  {
      start.WriteU16 (docList[i].length ());
      start.Write ((uint8_t *) (const_cast<char*> (docList[i].c_str())), docList[i].length());
  }
}

uint32_t
PennSearchMessage::SearchComplete::Deserialize (Buffer::Iterator &start)
{
  uint16_t length;
  uint16_t vectorSize = start.ReadU16 ();
  for(uint16_t i=0; i<vectorSize; i++)
  {
      length = start.ReadU16 ();
      char* str = (char*) malloc (length);
      start.Read ((uint8_t*)str, length);
      keyList.push_back(std::string (str, length));
      free (str);
  }

  vectorSize = start.ReadU16 ();
  for(uint16_t i=0; i<vectorSize; i++)
  {
      length = start.ReadU16 ();
      char* str = (char*) malloc (length);
      start.Read ((uint8_t*)str, length);
      docList.push_back(std::string (str, length));
      free (str);
  }
  return SearchComplete::GetSerializedSize ();
}

void
PennSearchMessage::SetSearchComplete (std::vector<std::string> keyList, std::vector<std::string> docList)
{
  if (m_messageType == 0)
    {
      m_messageType = SEARCH_COMPLETE;
    }
  else
    {
      NS_ASSERT (m_messageType == SEARCH_COMPLETE);
    }
  m_message.searchComplete.keyList = keyList;
  m_message.searchComplete.docList = docList;
}

PennSearchMessage::SearchComplete
PennSearchMessage::GetSearchComplete ()
{
  return m_message.searchComplete;
}

/* PASS_KEYS */

uint32_t
PennSearchMessage::PassKeys::GetSerializedSize (void) const
{
    uint32_t size;
    size = sizeof(uint16_t) + key.length() + sizeof(uint16_t);
    for(uint16_t i=0;i<docVector.size();i++)
    {
        size+= sizeof(uint16_t) + docVector[i].length();
    }
    return size;
}

void
PennSearchMessage::PassKeys::Print (std::ostream &os) const
{
    os << "PassKeys Key" << key << "\n";
}

void
PennSearchMessage::PassKeys::Serialize (Buffer::Iterator &start) const
{
    start.WriteU16 (key.length ());
    start.Write ((uint8_t *) (const_cast<char*> (key.c_str())), key.length());
    start.WriteU16 (docVector.size());
    for(uint16_t i=0;i<docVector.size();i++)
    {
        start.WriteU16 (docVector[i].length ());
        start.Write ((uint8_t *) (const_cast<char*> (docVector[i].c_str())), docVector[i].length());
    }
}

uint32_t
PennSearchMessage::PassKeys::Deserialize (Buffer::Iterator &start)
{
    uint16_t length = start.ReadU16 ();
    char* str = (char*) malloc (length);
    start.Read ((uint8_t*)str, length);
    key = std::string (str, length);
    free (str);

    uint16_t vectorSize = start.ReadU16 ();
    for(uint16_t i=0; i<vectorSize; i++)
    {
        length = start.ReadU16 ();
        char* str = (char*) malloc (length);
        start.Read ((uint8_t*)str, length);
        docVector.push_back(std::string (str, length));
        free (str);
    }
    return PassKeys::GetSerializedSize ();
}

void
PennSearchMessage::SetPassKeys (std::string key, std::vector<std::string> docVector)
{
  if (m_messageType == 0)
    {
      m_messageType = PASS_KEYS;
    }
  else
    {
      NS_ASSERT (m_messageType == PASS_KEYS);
    }
  m_message.passKeys.key = key;
  m_message.passKeys.docVector = docVector;
}

PennSearchMessage::PassKeys
PennSearchMessage::GetPassKeys ()
{
  return m_message.passKeys;
}


//
//
//

void
PennSearchMessage::SetMessageType (MessageType messageType)
{
  m_messageType = messageType;
}

PennSearchMessage::MessageType
PennSearchMessage::GetMessageType () const
{
  return m_messageType;
}

void
PennSearchMessage::SetTransactionId (uint32_t transactionId)
{
  m_transactionId = transactionId;
}

uint32_t 
PennSearchMessage::GetTransactionId (void) const
{
  return m_transactionId;
}

