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

#ifndef PENN_SEARCH_MESSAGE_H
#define PENN_SEARCH_MESSAGE_H

#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/packet.h"
#include "ns3/object.h"
#include <map>

using namespace ns3;

#define IPV4_ADDRESS_SIZE 4

class PennSearchMessage : public Header
{
  public:
    PennSearchMessage ();
    virtual ~PennSearchMessage ();


    enum MessageType
      {
        PING_REQ = 1,
        PING_RSP = 2,
	STORE_LIST = 3,
	SEARCH_INITIAL = 4,
	SEARCH_BEGIN = 5,
	SEARCH = 6,
	SEARCH_COMPLETE = 7,
	PASS_KEYS = 8,
        // Define extra message types when needed       
      };

    PennSearchMessage (PennSearchMessage::MessageType messageType, uint32_t transactionId);

    /**
    *  \brief Sets message type
    *  \param messageType message type
    */
    void SetMessageType (MessageType messageType);

    /**
     *  \returns message type
     */
    MessageType GetMessageType () const;

    /**
     *  \brief Sets Transaction Id
     *  \param transactionId Transaction Id of the request
     */
    void SetTransactionId (uint32_t transactionId);

    /**
     *  \returns Transaction Id
     */
    uint32_t GetTransactionId () const;

  private:
    /**
     *  \cond
     */
    MessageType m_messageType;
    uint32_t m_transactionId;
    /**
     *  \endcond
     */
  public:
    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId (void) const;
    void Print (std::ostream &os) const;
    uint32_t GetSerializedSize (void) const;
    void Serialize (Buffer::Iterator start) const;
    uint32_t Deserialize (Buffer::Iterator start);

    
    struct PingReq
      {
        void Print (std::ostream &os) const;
        uint32_t GetSerializedSize (void) const;
        void Serialize (Buffer::Iterator &start) const;
        uint32_t Deserialize (Buffer::Iterator &start);
        // Payload
        std::string pingMessage;
      };

    struct PingRsp
      {
        void Print (std::ostream &os) const;
        uint32_t GetSerializedSize (void) const;
        void Serialize (Buffer::Iterator &start) const;
        uint32_t Deserialize (Buffer::Iterator &start);
        // Payload
        std::string pingMessage;
      };
      
    struct StoreList
      {
	void Print (std::ostream &os) const;
	uint32_t GetSerializedSize (void) const;
	void Serialize (Buffer::Iterator &start) const;
	uint32_t Deserialize (Buffer::Iterator &start);
	// Payload
	std::string key;
	std::vector<std::string> docVector;
      };
    struct SearchInitial
      {
	void Print (std::ostream &os) const;
	uint32_t GetSerializedSize (void) const;
	void Serialize (Buffer::Iterator &start) const;
	uint32_t Deserialize (Buffer::Iterator &start);
	// Payload
	Ipv4Address initiatorAddress;
	std::vector<std::string> keyList;
      };
    struct SearchBegin
      {
	void Print (std::ostream &os) const;
	uint32_t GetSerializedSize (void) const;
	void Serialize (Buffer::Iterator &start) const;
	uint32_t Deserialize (Buffer::Iterator &start);
	// Payload
	Ipv4Address initiatorAddress;
	std::vector<std::string> keyList;
	std::vector<std::string> docList;
      };
    struct Search
      {
	void Print (std::ostream &os) const;
	uint32_t GetSerializedSize (void) const;
	void Serialize (Buffer::Iterator &start) const;
	uint32_t Deserialize (Buffer::Iterator &start);
	// Payload
	Ipv4Address initiatorAddress;
	std::vector<std::string> keyList;
	std::vector<std::string> docList;
      };
    struct SearchComplete
      {
	void Print (std::ostream &os) const;
	uint32_t GetSerializedSize (void) const;
	void Serialize (Buffer::Iterator &start) const;
	uint32_t Deserialize (Buffer::Iterator &start);
	// Payload
	std::vector<std::string> keyList;
	std::vector<std::string> docList;
      };
   struct PassKeys
      {
	void Print (std::ostream &os) const;
	uint32_t GetSerializedSize (void) const;
	void Serialize (Buffer::Iterator &start) const;
	uint32_t Deserialize (Buffer::Iterator &start);
	// Payload
	std::string key;
	std::vector<std::string> docVector;
      };


  private:
    struct
      {
        PingReq pingReq;
        PingRsp pingRsp;
	StoreList storeList;
	SearchInitial searchInitial;
	SearchBegin searchBegin;
	Search search;
	SearchComplete searchComplete;
	PassKeys passKeys;
      } m_message;
    
  public:
    /**
     *  \returns PingReq Struct
     */
    PingReq GetPingReq ();

    /**
     *  \brief Sets PingReq message params
     *  \param message Payload String
     */

    void SetPingReq (std::string message);

    /**
     * \returns PingRsp Struct
     */
    PingRsp GetPingRsp ();
    /**
     *  \brief Sets PingRsp message params
     *  \param message Payload String
     */
    void SetPingRsp (std::string message);
    
    void SetStoreList (std::string key, std::vector<std::string> docVector);
    StoreList GetStoreList ();
    
    void SetSearchInitial (Ipv4Address initiatorAddress, std::vector<std::string> keyList);
    SearchInitial GetSearchInitial ();

    void SetSearchBegin (Ipv4Address initiatorAddress, std::vector<std::string> keyList, std::vector<std::string> docList);
    SearchBegin GetSearchBegin ();
    
    void SetSearch (Ipv4Address initiatorAddress, std::vector<std::string> keyList, std::vector<std::string> docList);
    Search GetSearch ();
    
    void SetSearchComplete (std::vector<std::string> keyList, std::vector<std::string> docList);
    SearchComplete GetSearchComplete ();
    
    void SetPassKeys (std::string key, std::vector<std::string> docVector);
    PassKeys GetPassKeys ();


}; // class PennSearchMessage

static inline std::ostream& operator<< (std::ostream& os, const PennSearchMessage& message)
{
  message.Print (os);
  return os;
}

#endif
