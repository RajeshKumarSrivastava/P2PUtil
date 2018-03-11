// MYTCPSocket.cpp: implementation of the CMYTCPSocket class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include <winsock2.h>
//#include "MSTcpIp.h"


#include "MYSockets.h"
#define TCP_BUFFER_SIZE 512

#define DETERMINE_TIMEVAL(timeval, pTimeval, dwTimeout) \
    if (dwTimeout == INFINITE)\
    {\
        pTimeval = NULL;\
    }\
    else\
    {\
        timeval.tv_sec=dwTimeout/1000;\
        timeval.tv_usec=(dwTimeout%1000)*1000;\
        pTimeval = &timeval;\
    }

CMYSocket::CMYSocket(int iType)
{
    UtilitiesInit();
    memset(&m_LocalAddress, 0, sizeof(SOCKADDR_IN));
    memset(&m_RemoteAddress, 0, sizeof(SOCKADDR_IN));
    m_LocalAddress.sin_family = AF_INET;
    m_RemoteAddress.sin_family = AF_INET;
    m_bConnected = FALSE;
    m_bBound = FALSE;
    m_bOpen = FALSE;
    m_iType = iType;
    m_bShouldLinger = FALSE;
    m_pcsAccessSocket = new CCriticalSection();
    
    OpenSocket();
}

CMYSocket::CMYSocket(int iType, SOCKET hSocket)
{
    UtilitiesInit();
    m_hSocket = hSocket;
    memset(&m_LocalAddress, 0, sizeof(SOCKADDR_IN));
    memset(&m_RemoteAddress, 0, sizeof(SOCKADDR_IN));
    m_LocalAddress.sin_family = AF_INET;
    m_RemoteAddress.sin_family = AF_INET;
    m_bConnected = FALSE;
    m_bBound = FALSE;
    m_bOpen = hSocket != INVALID_SOCKET;
    m_iType = iType;
    m_bShouldLinger = FALSE;
    m_pcsAccessSocket = new CCriticalSection();
}
BOOL CMYSocket::UtilitiesInit(void)
{
    WORD wVersion = MAKEWORD(1, 1);
    WSADATA WSAData;
    BOOL bResult = FALSE;
    if (0 == WSAStartup(wVersion, &WSAData))
    {
        bResult = TRUE;
    }
    return bResult;
}
BOOL CMYSocket::OpenSocket(void)
{
    if (m_bOpen)
    {
        CloseSocket();
    }
    
    m_hSocket = socket(AF_INET, m_iType, NULL);
    if (m_hSocket != INVALID_SOCKET)
    {
        ULONG ulNonBlocking = 1;
        ioctlsocket(m_hSocket, FIONBIO, &ulNonBlocking);
        
        int iBroadcast = 1;
        setsockopt(m_hSocket, SOL_SOCKET, SO_BROADCAST, (char*)&iBroadcast, sizeof(int));

        bool iNagleValue = 1;           // Turn off NAGLE algorithm
        setsockopt(m_hSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&iNagleValue, sizeof(bool));

        WSAIoctl(0, 0, NULL, 0, NULL, 0, NULL, NULL, NULL);
        m_bOpen = TRUE;
    } 
    else 
    {
        m_bOpen = FALSE;
        int nRet;
        
        nRet = WSAGetLastError();
    }
    
    return (m_bOpen);
}

CMYSocket::~CMYSocket()
{
    CloseSocket();
    delete m_pcsAccessSocket;
    m_pcsAccessSocket = NULL;
}

void CMYSocket::CloseSocket(void)
{
    if (m_hSocket != INVALID_SOCKET)
    {
        closesocket(m_hSocket);
        m_hSocket = INVALID_SOCKET;
        m_bOpen = FALSE;
    }
    else
    {
        m_bOpen = FALSE;
        m_bConnected = FALSE;
    }
    
    return;
}

BOOL CMYSocket::Bind(ULONG ulAddress, DWORD dwPort)
{
    int iSize;
    
    memset(&m_LocalAddress, 0, sizeof(SOCKADDR_IN));
    m_LocalAddress.sin_family = AF_INET;
    m_LocalAddress.sin_addr.S_un.S_addr = ulAddress;
    m_LocalAddress.sin_port = htons((USHORT)dwPort);
    
    if (bind(m_hSocket, (SOCKADDR *)&m_LocalAddress, sizeof(SOCKADDR)) != SOCKET_ERROR)
    {
        m_bBound = TRUE;
        iSize = sizeof(m_LocalAddress);
        getsockname(m_hSocket, (SOCKADDR *)&m_LocalAddress, &iSize);
    }
    
    return (m_bBound);
}

BOOL CMYSocket::Bind(DWORD dwPort)
{
    ULONG ulAddress = INADDR_ANY;
    
    return (Bind(ulAddress, dwPort));
}

BOOL CMYSocket::Bind(void)
{
    if (bind(m_hSocket, (SOCKADDR *)&m_LocalAddress, sizeof(SOCKADDR)) != SOCKET_ERROR)
    {
        m_bBound = TRUE;
    }
    
    return (m_bBound);
}

BOOL CMYSocket::ResolveRemoteAddress(CHAR *szServerName)
{
    DWORD dwIPAddress = 0;
    BOOL bReturnValue = FALSE;
    
    // Try the server name as an IP Address first
    dwIPAddress = inet_addr(szServerName);
    
    // Did it resolve to a valid IP Address?
    if (dwIPAddress != INADDR_NONE)
    {
        m_RemoteAddress.sin_addr.s_addr = dwIPAddress;
        bReturnValue = TRUE;
    }
    
    return (bReturnValue);
}

BOOL CMYSocket::ResolveLocalAddress(CHAR *szServerName)
{
    DWORD dwIPAddress = 0;
    BOOL bReturnValue = FALSE;
    
    // Try the server name as an IP Address first
    dwIPAddress = inet_addr(szServerName);
    
    // Did it resolve to a valid IP Address?
    if (dwIPAddress != INADDR_NONE)
    {
        m_LocalAddress.sin_addr.s_addr = dwIPAddress;
        bReturnValue = TRUE;
    }
    
    return (bReturnValue);
}

BOOL CMYSocket::SetRemotePort(USHORT usPort)
{
    m_RemoteAddress.sin_port = htons(usPort);
    return (TRUE);
}

CMYTCPSocket::CMYTCPSocket(SOCKET hSocket) : CMYSocket(SOCK_STREAM, hSocket)
{
    m_bConnected = TRUE;
    m_bBound = TRUE;
    m_iBufPos = 0;
    m_iBufSize = 0;
    memset(m_btBuffer,0,TCP_BUFFER_SIZE);
}

CMYTCPSocket::CMYTCPSocket() : CMYSocket(SOCK_STREAM)
{
    m_iBufPos = 0;
    m_iBufSize = 0;
    memset(m_btBuffer,0,TCP_BUFFER_SIZE);
}

CMYTCPSocket::~CMYTCPSocket()
{
    //Disconnect();
}


BOOL CMYTCPSocket::Disconnect()
{

    m_bConnected = FALSE;

    return(TRUE);
}

BOOL CMYTCPSocket::IsConnected()
{
    int iBytesRead;
    DWORD dwRetVal = 0;
    fd_set ReadSet;
    CHAR cData[1];
    DWORD dwError;
    TIMEVAL timeout, *pTimeout;
    DETERMINE_TIMEVAL(timeout, pTimeout, 0)
    
    m_pcsAccessSocket->Lock();

    if (m_bConnected && (m_iBufSize <= 0))
    {
        FD_ZERO(&ReadSet);
        FD_SET(m_hSocket, &ReadSet);
        
        if (select(0, &ReadSet, NULL, NULL, pTimeout) >= 0)
        {
            if (m_hSocket == INVALID_SOCKET)
            {
                WSASetLastError(WSAECONNABORTED);
            }
            else if (FD_ISSET(m_hSocket, &ReadSet))
            {
                iBytesRead = recv(m_hSocket, (char *)cData, 1, MSG_PEEK);
                if (iBytesRead <= 0)
                {
                    dwError = WSAGetLastError();
                    if ((dwError != WSAEWOULDBLOCK) && (dwError != WSAEOPNOTSUPP))
                    {
                        Disconnect();
                    }
                }
            }
            else
            {
                WSASetLastError(WSAETIMEDOUT);
            }
        }
    }

    m_pcsAccessSocket->Unlock();

    return (m_bConnected);
}

BOOL CMYTCPSocket::Connect(DWORD dwTimeout)
{
    BOOL bRetVal = FALSE;
    DWORD dwReturn;
    m_iBufSize = 0;
    m_iBufPos = 0;
    memset(m_btBuffer,0,TCP_BUFFER_SIZE);
    
    if (m_bConnected)
    {
        bRetVal = TRUE;
    }
    else
    {
        if (!m_bOpen)
        {
            OpenSocket();
        }

        dwReturn = connect(m_hSocket, (SOCKADDR *)&m_RemoteAddress, sizeof(SOCKADDR));
        if ((dwReturn == SOCKET_ERROR) && (WSAGetLastError() == WSAEWOULDBLOCK))
        {
            if (dwTimeout)
            {
                bRetVal = WaitForConnection(dwTimeout);
            }
            else
            {
                bRetVal = TRUE;
            }
        }
        else if (!dwReturn)
        {
            m_bConnected = TRUE;
            bRetVal = TRUE;
        }
        
    }
    
    return (bRetVal);
}

BOOL CMYTCPSocket::WaitForConnection(DWORD dwTimeout)
{
    BOOL bRetVal = FALSE;
    fd_set WriteSet, ErrorSet;
    
    TIMEVAL timeout, *pTimeout;
    DETERMINE_TIMEVAL(timeout, pTimeout, dwTimeout)
    
    m_pcsAccessSocket->Lock();
    
    FD_ZERO(&WriteSet);
    FD_ZERO(&ErrorSet);
    FD_SET(m_hSocket, &WriteSet);
    FD_SET(m_hSocket, &ErrorSet);

    int rv = select(0, NULL, &WriteSet, &ErrorSet, pTimeout);

    if (rv > 0)
    {
        if (m_hSocket == INVALID_SOCKET)
        {
            WSASetLastError(WSAECONNABORTED);
        }
        else if (FD_ISSET(m_hSocket, &WriteSet))
        {
			WSASetLastError(ERROR_SUCCESS);
            m_bConnected = TRUE;
            bRetVal = TRUE;
        }  
    }
    else if (rv != SOCKET_ERROR)
    {
        WSASetLastError(WSAETIMEDOUT);
    }

    m_pcsAccessSocket->Unlock();
    
    return (bRetVal);
}

DWORD CMYTCPSocket::ReadChar(BYTE *sChar,DWORD dwTimeout)
{
    DWORD dwRetVal = 0;
    fd_set ReadSet;
    
    TIMEVAL timeout, *pTimeout;
    DETERMINE_TIMEVAL(timeout, pTimeout, dwTimeout)
    
    m_pcsAccessSocket->Lock();

    //if we've run out of data in our buffer, read more from the socket
    if (m_iBufPos >= m_iBufSize)
    {
        m_iBufPos = 0;
        m_iBufSize = 0;
        
        if (m_bConnected)
        {
            FD_ZERO(&ReadSet);
            FD_SET(m_hSocket, &ReadSet);
            
            if (select(0, &ReadSet, NULL, NULL, pTimeout) >= 0)
            {
                if (m_hSocket == INVALID_SOCKET)
                {
                    WSASetLastError(WSAECONNABORTED);
                }
                else if (FD_ISSET(m_hSocket, &ReadSet))
                {
                    m_iBufSize = recv(m_hSocket, (char *)m_btBuffer, TCP_BUFFER_SIZE, 0);
                    m_iBufPos = 0;
                    //if there wasn't any data rcvd, disconnect
                    if (m_iBufSize <= 0)
                        Disconnect();
                }
            }
            else
            {
                WSASetLastError(WSAETIMEDOUT);
            }
        }
    }

    //if there's any data available, return the next character
    if (m_iBufSize > 0)
    {
        *sChar = m_btBuffer[m_iBufPos];
        ++m_iBufPos;
        dwRetVal = 1;
    }

        m_pcsAccessSocket->Unlock();
    
    return (dwRetVal);
}

DWORD CMYTCPSocket::Read(BYTE *pbtBuffer, DWORD dwLength, DWORD dwTimeout)
{
    int iBytesRead = 0;
    DWORD dwRetVal = 0;
    fd_set ReadSet;
    DWORD dwOffset = 0;
    
    TIMEVAL timeout, *pTimeout;
    DETERMINE_TIMEVAL(timeout, pTimeout, dwTimeout)

    m_pcsAccessSocket->Lock();

    //check if there is any data left in the buffer, if so, copy that first before reading anything from the socket
    if (m_iBufSize > m_iBufPos)
    {
        dwOffset = m_iBufSize-m_iBufPos;
        //check to make sure the output buffer is large enough to hold all the data
        if (dwOffset > dwLength)
        {
            dwOffset = dwLength;
        }
        
        memcpy(pbtBuffer,&m_btBuffer[m_iBufPos],dwOffset);
        
        //if we've copied all the data from the buffer, clear out the size and position
        dwRetVal = dwOffset;
        m_iBufPos += dwOffset;
        if (m_iBufPos >= m_iBufSize)
        {
            m_iBufSize = 0;
            m_iBufPos = 0;
        }
        else 
        {
            //there's no more room in the output buffer so don't bother trying to read anything from the socket
            m_pcsAccessSocket->Unlock();
            return dwRetVal;
        }
    }
    
    if (m_bConnected)
    {
        FD_ZERO(&ReadSet);
        FD_SET(m_hSocket, &ReadSet);

        if (select(0, &ReadSet, NULL, NULL, pTimeout) >= 0)
        {
            if (m_hSocket == INVALID_SOCKET)
            {
                WSASetLastError(WSAECONNABORTED);
            }
            else if (FD_ISSET(m_hSocket, &ReadSet))
            {
                iBytesRead = recv(m_hSocket, (char *)(pbtBuffer+dwOffset), dwLength-dwOffset, 0);
                if (iBytesRead > 0)
                {
                    dwRetVal += iBytesRead;
                }
                else
                {
                    Disconnect();
                }
            }
            else
            {
                WSASetLastError(WSAETIMEDOUT);
            }
        }
    }

    m_pcsAccessSocket->Unlock();
    
    return (dwRetVal);
}

// Read full amount of data, waiting and retrying if necessary up to a timeout.
// Returns amount of data received.
DWORD CMYTCPSocket::ReadFullAmount(BYTE *pbtBuffer, DWORD dwLength, DWORD dwTimeout, int flags)
{
    int     amountleft;
    int     amountread;
    int     rv;
    FD_SET  ReadSet;
    BOOL    bError = FALSE;         // set to true if an error occurs
    
    TIMEVAL timeout, *pTimeout;
    DETERMINE_TIMEVAL(timeout, pTimeout, dwTimeout)

    amountleft=dwLength;
    amountread=0;

    while ((amountleft > 0) && (!bError)) 
    {
        __try 
        {
            FD_ZERO(&ReadSet);
            FD_SET(m_hSocket,&ReadSet);

            rv = select(0, &ReadSet, NULL, NULL, pTimeout);
            
            if (rv > 0 && m_hSocket != INVALID_SOCKET && FD_ISSET(m_hSocket, &ReadSet))
            {
                amountread=recv(m_hSocket,(char *)pbtBuffer+dwLength-amountleft,amountleft,flags);
            } 
            else if (!rv)   // timeout
            {    
                amountread=0;
                WSASetLastError(WSAETIMEDOUT);
            } 
            else 
            {
                amountread = 0;
            }
        } 
        __except(1) 
        {
            amountread = 0;
        }
        
        if ((0 == amountread) || (SOCKET_ERROR == amountread))     
        {
            bError = TRUE;   
            amountread = 0;
        }
        
        amountleft -= amountread;
    }
    
    return (dwLength - amountleft);
}

DWORD CMYTCPSocket::Write(BYTE *pbtBuffer, DWORD dwLength)
{
    DWORD dwRetVal = 0;
    int iRet;
    
    m_pcsAccessSocket->Lock();
    
    if (m_bConnected)
    {
        iRet = send(m_hSocket, (char *)pbtBuffer, dwLength, 0);
        
        if (iRet <= 0)
        {
            Disconnect();
        }
        else
        {
            m_bShouldLinger = TRUE;
            dwRetVal = (DWORD) iRet;
        }
    }
    
    m_pcsAccessSocket->Unlock();
    
    return (dwRetVal);
}

CMYTCPServerSocket::CMYTCPServerSocket() : CMYSocket(SOCK_STREAM)
{
    m_bIsListening = FALSE;
}

CMYTCPServerSocket::~CMYTCPServerSocket()
{
    Disconnect();
}


BOOL CMYTCPServerSocket::Disconnect()
{
    m_bConnected = FALSE;
    
    return (TRUE);
}

BOOL CMYTCPServerSocket::IsConnected()
{
    int iBytesRead;
    DWORD dwRetVal = 0;
    fd_set ReadSet;
    CHAR cData[1];
    DWORD dwError;
    TIMEVAL timeout, *pTimeout;
    DETERMINE_TIMEVAL(timeout, pTimeout, 0)

    m_pcsAccessSocket->Lock();
    if (m_bConnected)
    {
        FD_ZERO(&ReadSet);
        FD_SET(m_hSocket, &ReadSet);
        
        if (select(0, &ReadSet, NULL, NULL, pTimeout) >= 0)
        {
            if (m_hSocket == INVALID_SOCKET)
            {
                WSASetLastError(WSAECONNABORTED);
            }
            else if (FD_ISSET(m_hSocket, &ReadSet))
            {
                iBytesRead = recv(m_hSocket, (char *)cData, 1, MSG_PEEK);
                if (iBytesRead <= 0)
                {
                    dwError = WSAGetLastError();
                    if ((dwError != WSAEWOULDBLOCK) && (dwError != WSAEOPNOTSUPP))
                    {
                        Disconnect();
                    }
                }
            }
            else
            {
                WSASetLastError(WSAETIMEDOUT);
            }
        }
    }
    
    m_pcsAccessSocket->Unlock();

    return (m_bConnected);
}

BOOL CMYTCPServerSocket::IsListening(void)
{
    return (m_bIsListening);
}

BOOL CMYTCPServerSocket::Listen(DWORD dwTimeout)
{
    BOOL bRetVal = FALSE;
    DWORD dwReturn;
    
    if (m_bConnected)
    {
        bRetVal = TRUE;
    }
    else
    {
        dwReturn = listen(m_hSocket, 5);
        if (dwReturn == 0)
        {
            m_bIsListening = TRUE;
            if (dwTimeout)
            {
                bRetVal = WaitForConnection(dwTimeout);
            }
            else
            {
                bRetVal = TRUE;
            }
        }
    }
    
    return (bRetVal);
}

BOOL CMYTCPServerSocket::WaitForConnection(DWORD dwTimeout)
{
    BOOL bRetVal = FALSE;
    fd_set ReadSet, ErrorSet;
    TIMEVAL timeout, *pTimeout;
    DETERMINE_TIMEVAL(timeout, pTimeout, dwTimeout)
    
    m_pcsAccessSocket->Lock();
    FD_ZERO(&ReadSet);
    FD_SET(m_hSocket, &ReadSet);

    FD_ZERO(&ErrorSet);
    FD_SET(m_hSocket, &ErrorSet);

    int rv = select(0, &ReadSet, NULL, &ErrorSet, pTimeout);

    if (rv > 0)
    {
        if (m_hSocket == INVALID_SOCKET)
        {
            WSASetLastError(WSAECONNABORTED);
        } 
        else if (FD_ISSET(m_hSocket, &ReadSet))
        {
            int iSize = sizeof(m_RemoteAddress);
            m_hConnectedSocket = accept(m_hSocket, (SOCKADDR *)&m_RemoteAddress, &iSize);
            if (m_hConnectedSocket != INVALID_SOCKET) 
            {
                m_bConnected = TRUE;
                bRetVal = TRUE;
            }
        }
    }
    else if (rv == SOCKET_ERROR)
    {
        WSASetLastError(WSAETIMEDOUT);
    }

    m_pcsAccessSocket->Unlock();
    
    return (bRetVal);
}

BOOL CMYTCPServerSocket::PopulateSocketFromNewConnection(CMYTCPSocket *pSocket)
{
    BOOL bRetVal = FALSE;
    int iSize;
    
    m_pcsAccessSocket->Lock();

    if (m_bConnected)
    {
        bRetVal = TRUE;
        if (pSocket)
        {
            pSocket->m_hSocket = m_hConnectedSocket;
            pSocket->m_bConnected = TRUE;
            
            iSize = sizeof(pSocket->m_LocalAddress);
            getsockname(m_hConnectedSocket, (SOCKADDR *)&pSocket->m_LocalAddress, &iSize);
            
            iSize = sizeof(pSocket->m_RemoteAddress);
            getpeername(m_hConnectedSocket, (SOCKADDR *)&pSocket->m_RemoteAddress, &iSize);
            
            pSocket->m_iType = m_iType;
            pSocket->m_bBound = m_bBound;
            pSocket->m_bOpen = m_bOpen;
        }
        
        m_hConnectedSocket = INVALID_SOCKET;
        m_bConnected = FALSE;
    }
    
    m_pcsAccessSocket->Unlock();

    return bRetVal;
}

SOCKET CMYTCPServerSocket::GetConnectedSocket(void)
{
    SOCKET hConnectedSocket = INVALID_SOCKET;
    
    m_pcsAccessSocket->Lock();

    if (m_bConnected)
    {
        hConnectedSocket = m_hConnectedSocket;
        m_hConnectedSocket = INVALID_SOCKET;
        m_bConnected = FALSE;
    }
    
    m_pcsAccessSocket->Unlock();

    return hConnectedSocket;
}

DWORD CMYTCPServerSocket::Read(BYTE *pbtBuffer, DWORD dwLength, DWORD dwTimeout)
{
    DWORD dwRetVal = 0;
    fd_set ReadSet;
    TIMEVAL timeout, *pTimeout;
    DETERMINE_TIMEVAL(timeout, pTimeout, dwTimeout);
    int iWSAError = 0;
    
    m_pcsAccessSocket->Lock();

    if (m_bConnected)
    {
        FD_ZERO(&ReadSet);
        FD_SET(m_hConnectedSocket, &ReadSet);
        
        if (select(0, &ReadSet, NULL, NULL, pTimeout) >= 0)
        {
            if (m_hConnectedSocket == INVALID_SOCKET)
            {
                WSASetLastError(WSAECONNABORTED);
            } 
            else if (FD_ISSET(m_hConnectedSocket, &ReadSet))
            {
                int iBytesRead = recv(m_hConnectedSocket, (char *)pbtBuffer, dwLength, 0);
                if (iBytesRead > 0)
                {
                    dwRetVal = iBytesRead;
                }
                else if (iBytesRead == SOCKET_ERROR)
                {
                    iWSAError = WSAGetLastError();
                    Disconnect();
                    WSASetLastError(iWSAError);
                }
                else
                {
                    // recv returned 0 so connection is closed.
                    Disconnect();
                }
            }
            else
            {
                WSASetLastError(WSAETIMEDOUT);
            }
	    }
    }
    else
    {
        WSASetLastError(WSAENOTCONN);
    }

    m_pcsAccessSocket->Unlock();
    
    return (dwRetVal);
}

DWORD CMYTCPServerSocket::ReadLine(BYTE *pbtBuffer, DWORD dwLength, DWORD dwTimeout)
{
    DWORD dwBytesRead = 0;
    int iCurrentBytes;
    BOOL bDone = FALSE;
    
    if (m_bConnected)
    {
        while (!bDone)
        {
            iCurrentBytes = Read(pbtBuffer+dwBytesRead, 1, dwTimeout);
            if (iCurrentBytes > 0)
            {
                dwBytesRead += iCurrentBytes;
                if (dwBytesRead >= dwLength)
                {
                    bDone = TRUE;
                }
                else if (dwBytesRead >= 1)
                {
                    if (memcmp(pbtBuffer+dwBytesRead-1, "\n", 1) == 0)
                    {
                        bDone = TRUE;
                        pbtBuffer[dwBytesRead-1] = 0;
                        if ((dwBytesRead >= 2) && (pbtBuffer[dwBytesRead-2] == '\r'))
                        {
                            pbtBuffer[dwBytesRead-2] = 0;
                        }
                    }
                }
            }
            else
            {
                bDone = TRUE;
            }
        }
    }
    
    return (dwBytesRead);
}

DWORD CMYTCPServerSocket::Write(BYTE *pbtBuffer, DWORD dwLength)
{
    DWORD dwRetVal = 0;
    int iRet;
    
    m_pcsAccessSocket->Lock();

    if (m_bConnected)
    {
        iRet = send(m_hConnectedSocket, (char *)pbtBuffer, dwLength, 0);
    
        if (iRet == SOCKET_ERROR)
        {
            iRet = WSAGetLastError();
            Disconnect();
            WSASetLastError(iRet);
        }
        else
        {
            dwRetVal = (DWORD) iRet;
            m_bShouldLinger = TRUE;
        }
    }
    else
    {
        WSASetLastError(WSAETIMEDOUT);
    }

    m_pcsAccessSocket->Unlock();
    
    return (dwRetVal);
}

