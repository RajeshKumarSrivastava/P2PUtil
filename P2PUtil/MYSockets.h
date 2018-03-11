#ifndef __MY_SOCKET_H
#define __MY_SOCKET_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>

// There is a general purpose socket which is further extended to a TCP client and TCP server sockets

class CMYSocket
{
public:
    CMYSocket(int iType);
    CMYSocket(int iType, SOCKET hSocket);
    ~CMYSocket(void);

    BOOL        OpenSocket(void);
    void        CloseSocket(void);

    BOOL   Bind(void);
    BOOL   Bind(DWORD dwPort);
    BOOL   Bind(ULONG ulAddress, DWORD dwPort);
    BOOL   ResolveRemoteAddress(CHAR *szServerName);
    BOOL   ResolveLocalAddress(CHAR *szServerName);
    BOOL   SetRemotePort(USHORT usPort);
    BOOL	UtilitiesInit(void);

protected:
    CCriticalSection *m_pcsAccessSocket;

private:
    SOCKET      m_hSocket;
    SOCKADDR_IN m_LocalAddress;
    SOCKADDR_IN m_RemoteAddress;
    BOOL        m_bConnected;
    BOOL        m_bBound;
    BOOL        m_bOpen;
    int         m_iType;
    int         m_iLastError;
    BOOL        m_bShouldLinger;

    friend class CMYTCPSocket;
    friend class CMYTCPServerSocket;
};

#define MY_TCP_BUFFER_SIZE		512
class CMYTCPSocket : public CMYSocket
{
public:
    CMYTCPSocket(void);
    CMYTCPSocket(SOCKET hSocket);
    ~CMYTCPSocket(void);

    BOOL Disconnect(void);
    BOOL IsConnected(void);
    BOOL Connect(DWORD dwTimeout);
    BOOL WaitForConnection(DWORD dwTimeout);
    DWORD Read(BYTE *pbtBuffer, DWORD dwLength, DWORD dwTimeout);
    DWORD ReadFullAmount(BYTE *pbtBuffer, DWORD dwLength, DWORD dwTimeout, int flags);
    DWORD ReadChar(BYTE *sChar,DWORD dwTimeout);
    DWORD Write(BYTE *pbtBuffer, DWORD dwLength);
  
private:
	BYTE m_btBuffer[MY_TCP_BUFFER_SIZE];
	int m_iBufPos;
	int m_iBufSize;
};

class CMYTCPServerSocket : public CMYSocket
{
public:
    CMYTCPServerSocket(void);
    ~CMYTCPServerSocket(void);

    BOOL Disconnect(void);
    BOOL IsConnected(void);
    BOOL IsListening(void);
    BOOL Listen(DWORD dwTimeout);
    BOOL WaitForConnection(DWORD dwTimeout);
    BOOL PopulateSocketFromNewConnection(CMYTCPSocket *pSocket);
    DWORD Read(BYTE *pbtBuffer, DWORD dwLength, DWORD dwTimeout);
    DWORD ReadLine(BYTE *pbtBuffer, DWORD dwLength, DWORD dwTimeout);
    DWORD Write(BYTE *pbtBuffer, DWORD dwLength);
    SOCKET GetConnectedSocket(void);
  
private:
    BOOL   m_bIsListening;
    SOCKET m_hConnectedSocket;
};


#endif 
