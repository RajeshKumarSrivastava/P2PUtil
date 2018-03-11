#include "Peer.h"

CPeer* CPeer::mst_peer = nullptr;

CPeer::CPeer() :m_SocketMgr(new CSocketManager(this))
{
	// TODO : Identity of this Peer
}

bool CPeer::sendMessage(BYTE* btBuffer, DWORD dwLength)
{

}

CPeer* CPeer::FactoryPeer()
{
	if (mst_peer == nullptr)
	{
		mst_peer = new CPeer();
	}
}

bool CPeer::RequestFile(std::string filename)
{

}

//Start the Main Server which will keep looking for Peer connect
bool CPeer::StartTheServer()
{

}
//Start the client which will allow the Peer to connect when it requires
bool CPeer::StartTheClient()
{

}