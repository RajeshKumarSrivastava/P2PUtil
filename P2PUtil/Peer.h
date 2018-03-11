#ifndef __PEER_H
#define __PEER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "stdafx.h" // You will find all the headers here, which qualify for pch
#include "CSocketManager.h"
#include "MsgQueue.h"

namespace {
	// Probably I thought and was trying to fix a key for encrypting a broadcast message for known peers
	// will try and extend it later
	static const BYTE BYTE_FIXED_KEY[] = { 0x2A, 0xA2, 0x3A, 0xA3 }; //secret
};

// CPeer represents one Peer and therefore restricted to one instance.
// Since this class is a Singelton, no need to define an interface for it ...

class CPeer {
public:
	CPeer();
	CPeer(const CPeer& rhs) = delete; // Not required
	CPeer& operator = (const CPeer& rhs) = delete; //Not required
	
	static CPeer* FactoryPeer();
	bool StartTheServer();
	bool StartTheClient();
	bool RequestFile(std::string strFileName); // Peer should ask the nearest(for now a known remote) Peer for info
	bool sendMessage(BYTE* byteBuffer, DWORD dwlength);
	BYTE* recvMessage();

private:
	static CPeer* mst_peer;
	CSocketManager* m_SocketMgr;
};
#endif
