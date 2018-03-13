// P2PUtil.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Peer.h"



int main()
{
	CPeer* Peer = CPeer::FactoryPeer();

	if (bool bServerStarted = Peer->StartTheServer())
	{
		//Log that the thread has started
	}
	else
	{
		// Log  and may be start again ...later
	}
	if (bool bClientStarted = Peer->StartTheClient())
	{
		//Log that the thread has started
		bool breturn = Peer->SendRequestFile("ABC.txt"); // Just to Demo both server and client works asysnchronously
		if (breturn)
		{
			//file requested
		}
		else
		{
			//May be request again
		}
	}
	else
	{
		// Log  and may be start again ...later
	}

    return 0;
}

