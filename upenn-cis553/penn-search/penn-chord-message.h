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

#ifndef PENN_CHORD_MESSAGE_H
#define PENN_CHORD_MESSAGE_H

#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/packet.h"
#include "ns3/object.h"

using namespace ns3;

#define IPV4_ADDRESS_SIZE 4

class PennChordMessage : public Header
{
  public:
    PennChordMessage ();
    virtual ~PennChordMessage ();


    enum MessageType
      {
        PING_REQ = 1,
        PING_RSP = 2,
        JOIN_CHORD = 3,
        FIND_SUCCESSOR = 4,
        JOIN_CHORD_SUCCESS = 5,
        JOIN_CHORD_FAIL = 6,
        NOTIFY = 7,
        STABILIZE_REQ = 8,
        STABILIZE_RESP = 9,
        RINGSTATE = 10,
        LEAVE_SUCCESSOR = 11,
        LEAVE_PREDECESSOR = 12,
	FIND_FINGER = 13,
	FIND_FINGER_SUCCESS = 14,
	LOOKUP_PUBLISH = 15,
	LOOKUP_PUBLISH_SUCCESS = 16
        // Define extra message types when needed       
      };

    PennChordMessage (PennChordMessage::MessageType messageType, uint32_t transactionId);

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
    
    struct JoinChord
      {
	void Print (std::ostream &os) const;
	uint32_t GetSerializedSize (void) const;
	void Serialize (Buffer::Iterator &start) const;
	uint32_t Deserialize (Buffer::Iterator &start);
	//Payload
      };
    
    struct FindSuccessor
      {
	void Print (std::ostream &os) const;
	uint32_t GetSerializedSize (void) const;
	void Serialize (Buffer::Iterator &start) const;
	uint32_t Deserialize (Buffer::Iterator &start);
	//Payload
        Ipv4Address destAddress;
	unsigned char targetDigest[20];
      };
      
    struct JoinChordSuccess
      {
	void Print (std::ostream &os) const;
	uint32_t GetSerializedSize (void) const;
	void Serialize (Buffer::Iterator &start) const;
	uint32_t Deserialize (Buffer::Iterator &start);
	//Payload
        Ipv4Address successorAddress;
      };

    struct StabilizeResp
      {
    void Print (std::ostream &os) const;
    uint32_t GetSerializedSize (void) const;
    void Serialize (Buffer::Iterator &start) const;
    uint32_t Deserialize (Buffer::Iterator &start);
    //Payload
        Ipv4Address predecessorAddress;
      };

    struct Ringstate
      {
    void Print (std::ostream &os) const;
    uint32_t GetSerializedSize (void) const;
    void Serialize (Buffer::Iterator &start) const;
    uint32_t Deserialize (Buffer::Iterator &start);
    //Payload
        Ipv4Address initiatorAddress;
      };
    struct LeaveSuccessor
      {
    void Print (std::ostream &os) const;
    uint32_t GetSerializedSize (void) const;
    void Serialize (Buffer::Iterator &start) const;
    uint32_t Deserialize (Buffer::Iterator &start);
    //Payload
        Ipv4Address predecessorAddress;
      };
    struct LeavePredecessor
      {
    void Print (std::ostream &os) const;
    uint32_t GetSerializedSize (void) const;
    void Serialize (Buffer::Iterator &start) const;
    uint32_t Deserialize (Buffer::Iterator &start);
    //Payload
        Ipv4Address successorAddress;
      };
    struct FindFinger
    {
      void Print (std::ostream &os) const;
      uint32_t GetSerializedSize (void) const;
      void Serialize (Buffer::Iterator &start) const;
      uint32_t Deserialize (Buffer::Iterator &start);
      //Payload
      Ipv4Address targetAddress;
      unsigned char targetDigest[20];
      uint16_t targetIndex;
    };
    struct FindFingerSuccess
    {
      void Print (std::ostream &os) const;
      uint32_t GetSerializedSize (void) const;
      void Serialize (Buffer::Iterator &start) const;
      uint32_t Deserialize (Buffer::Iterator &start);
      //Payload
      Ipv4Address fingerAddress;
      uint16_t fingerIndex;
    };
  struct LookupPublish
      {
	void Print (std::ostream &os) const;
	uint32_t GetSerializedSize (void) const;
	void Serialize (Buffer::Iterator &start) const;
	uint32_t Deserialize (Buffer::Iterator &start);
	//Payload
	uint16_t flag;
	Ipv4Address initiatorAddress;
	unsigned char lookupDigest[20];
	std::string lookupKey;
      };
  struct LookupPublishSuccess
      {
	void Print (std::ostream &os) const;
	uint32_t GetSerializedSize (void) const;
	void Serialize (Buffer::Iterator &start) const;
	uint32_t Deserialize (Buffer::Iterator &start);
	//Payload
	uint16_t flag;
	std::string lookupKey;
	Ipv4Address addressResponsible;
      };
  

    
  private:
    struct
      {
        PingReq pingReq;
        PingRsp pingRsp;
        JoinChord joinChord;
        FindSuccessor findSuccessor;
        JoinChordSuccess joinChordSuccess;
        StabilizeResp stabilizeResp;
        Ringstate ringstate;
        LeaveSuccessor leaveSuccessor;
        LeavePredecessor leavePredecessor;
	FindFinger findFinger;
	FindFingerSuccess findFingerSuccess;
	LookupPublish lookupPublish;
	LookupPublishSuccess lookupPublishSuccess;
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
    
    void SetFindSuccessor (Ipv4Address destAddr, unsigned char *m_digest);
    FindSuccessor GetFindSuccessor ();
  
    void SetJoinChordSuccess (Ipv4Address successorAddr);
    JoinChordSuccess GetJoinChordSuccess ();

    void SetStabilizeResp (Ipv4Address predecessorAddr);
    StabilizeResp GetStabilizeResp ();

    void SetRingstate (Ipv4Address initiatorAddr);
    Ringstate GetRingstate ();

    void SetLeaveSuccessor (Ipv4Address predecessorAddr);
    LeaveSuccessor GetLeaveSuccessor ();

    void SetLeavePredecessor (Ipv4Address successorAddr);
    LeavePredecessor GetLeavePredecessor ();
    
    void SetFindFinger (Ipv4Address targetAddr, unsigned char *targetDig, uint16_t targetInd);
    FindFinger GetFindFinger ();
    
    void SetFindFingerSuccess (Ipv4Address fingerAddr, uint16_t fingerInd);
    FindFingerSuccess GetFindFingerSuccess ();
    
    void SetLookupPublish (uint16_t flag, Ipv4Address initiatorAddr, unsigned char *lookupDig, std::string lookupKey);
    LookupPublish GetLookupPublish ();

    void SetLookupPublishSuccess (uint16_t flag, Ipv4Address addressResp, std::string lookupKey);
    LookupPublishSuccess GetLookupPublishSuccess ();


}; // class PennChordMessage

static inline std::ostream& operator<< (std::ostream& os, const PennChordMessage& message)
{
  message.Print (os);
  return os;
}

#endif
