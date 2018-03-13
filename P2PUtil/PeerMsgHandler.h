#pragma once
#include "stdafx.h"
#include <vector>

namespace {
	// Probably I thought and was trying to fix a key for encrypting a broadcast message for known peers
	// will try and extend it later
	static const BYTE BYTE_FIXED_KEY[] = { 0x2A, 0xA2, 0x3A, 0xA3 }; //secret
};

/* The Message structure is defined as
{ [0x00, 0x00, 0x3A, 0x01], // Header which signifies the full length of the Packet
[0xAA], // Data Type, If it's a command, a ACK, or a Data(the actual file stream)
[0xFF, 0xFF], //Length of the actual data, in case of command or Ack it should be just 1 BYTE
[0x01], //PeerCommand/Data -- In case of data this identifier will be the size of data
[0x2A, 0xA2, 0x3A, 0xA3]	// PAC(Peer Access Code) Signifies if the Packet comes from a valid Peer
}
*/
namespace PeerCommands {
	static const BYTE COMM_REQ_FILE			= 0x02; // request for a file
	static const BYTE COMM_REQ_RES_FILE_ACK = 0x20; // request response for a file

	static const BYTE COMM_SHARE_FILE		= 0X01; // just before sharing a file
	static const BYTE COMM_SHARE_FILE_ACK	= 0X10; // ready to recv file
	static const BYTE COMM_SHARE_FILE_NACK	= 0X00; // can't share
	static const BYTE COMM_SHARE_FILE_complete = 0X11; // sharing complete




	static const BYTE COMM_REQ_FILE_NACK	= 0x20; // remote sends and denies at the first place
	// So on and so forth.....
}


using Buffer = std::vector<BYTE>; //Better if we extend this vector under this Buffer


class CP2PProtocol
{
public:
	CP2PProtocol() {}
	CP2PProtocol(BYTE* btMessageBuffer, DWORD dwLength, bool bResponse = false); // is this a response?
	~CP2PProtocol() {}
	bool                        HandleOutgoingMessages(CP2PProtocol* pProtocol, BYTE *pbtDataPacket, DWORD dwMsgLength);

	virtual Buffer				GetMessageHeader() { return m_oHeaderPacket; }
	virtual Buffer				GetDataPacket() { return m_oDataPacket; }
	virtual Buffer				GetEtx() { return NULL; }
	inline BYTE                 GetCommandCode() { return m_btMessageType; }

private:
	virtual void                SetHeader(BYTE* btData, DWORD dwLength);
	virtual void                SetData(BYTE* btData, DWORD dwLength);
	virtual void                SetEtx(BYTE* btData, DWORD dwLength);

	DWORD                       m_dwMsgLength;
	Buffer						m_oDataPacket;
	Buffer						m_oHeaderPacket;
	BYTE                        m_btMessageType;
};


// 
class CP2PMessageHandler
{
public:
	CP2PMessageHandler() :{}
	virtual                     ~CP2PMessageHandler() {}

	CP2PProtocol*               Serealize(BYTE* btMessage, DWORD dwLength); // Goes to recv queue
	BYTE*                       Deserealize(BYTE btCommand, BYTE * pProtocolMessage);  // Push in send queue
	
	inline bool                 ClearQueues() { m_receiveMessageQueue.Clear(); m_sendingMessageQueue.Clear(); }
	inline CMsgQueue*			GetReceiveQueue(void) { return &m_receiveMessageQueue; }
	inline CMsgQueue*			GetSendingQueue(void) { return &m_sendingMessageQueue; }

	IProtocol*                  PopIncomingMsgFromQueue(void);
	IProtocol*                  PopOutgoingMsgFromQueue(void);


protected:
	CMsgQueue					m_receiveMessageQueue;      // Receive Message Queue
	CMsgQueue					m_sendingMessageQueue;      // Sending Message Queue
};



