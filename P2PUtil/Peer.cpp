#include "Peer.h"
#include "PeerMsgHandler.h"
#include "CSocketManager.h"

CPeer* CPeer::mst_peer = nullptr;

CPeer::CPeer() :m_SocketMgr(new CSocketManager(this)), m_p2pMsghandler(new CP2PMessageHandler())
{
}
CPeer::~CPeer()
{
	delete m_SocketMgr;
	delete m_p2pMsghandler;
}


CPeer* CPeer::FactoryPeer()
{
	if (mst_peer == nullptr)
	{
		mst_peer = new CPeer();
	}
	else
	{
		return mst_peer;
	}
	return mst_peer;
}

// Request to share a file that may be present at the immediate Peer.
bool CPeer::SendRequestFile(std::string filename)
{
	//send a request first
	if (m_p2pMsghandler->Deserealize(PeerCommands::COMM_REQ_FILE, reinterpret_cast<BYTE*>(const_cast<char*>(filename.c_str()))))
	{
		std::cout << "Message enqueued" << std::endl;
	}
	else
	{
		//Log message was not Deserialized
	}

}

//Start the Main Server which will keep looking for Peer connect
bool CPeer::StartTheServer()
{
	m_SocketMgr->CreateServerSocketThread();
}
//Start the client which will allow the Peer to connect when it requires
bool CPeer::StartTheClient()
{
	m_SocketMgr->CreateSocketThread();
}