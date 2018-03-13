#ifndef __PEER_H
#define __PEER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "stdafx.h" // You will find all the headers here, which qualify for pch

class CSocketManager;
class CP2PMessageHandler;

// CPeer represents one Peer and therefore restricted to one instance.
// Since this class is a Singelton, no need to define an interface for it ...

class CPeer {
public:
	CPeer(const CPeer& rhs) = delete; // Not required
	CPeer& operator = (const CPeer& rhs) = delete; //Not required
	~CPeer();
	
	static CPeer* FactoryPeer();
	bool StartTheServer();
	bool StartTheClient();
	bool SendRequestFile(std::string strFileName); // Peer should ask the nearest(for now a known remote) Peer for info


private:
	CPeer(); //restricted call
	static CPeer*			mst_peer;
	CSocketManager*			m_SocketMgr;     // Handles the communication Interface
	CP2PMessageHandler*		m_p2pMsghandler; // Handles the protocol messages
};
#endif
