#ifndef _C_SOCKET_MANAGER_H_
#define _C_SOCKET_MANAGER_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MsgQueue.h"

#define BUFFER_SIZE             4096
#define GENERIC_MSG_HEADER_SIZE 4

class CPeer;
class CSocketManager
{
public:
    // Constructors/Destructors
    explicit CSocketManager(CPeer* pPeer);
	CSocketManager() = delete;
	CSocketManager& operator=(CSocketManager& rGenericSocketManager) = delete;
	CSocketManager(CSocketManager& genericSocketManager) = delete;
    virtual ~CSocketManager(void);

    /// <summary> Check if socket is connected or not </summary>
    /// <return>  Returns true if connected else false <return>
    bool IsOpen();

    /// <summary> It will create a socket thread and then check for connection </summary>
    /// <return>  Returns true if thread is created else false <return>
    bool CreateSocketThread();

    /// <summary> It will close the connection, terminate socket thread 
    ///           And destroy synchronisation object </summary>
    /// <return>  Returns nothing <return>
    void StopComm();

    /// <summary> It will create a socket and connect to the configured
	///			  Peer IP </summary>
    /// <return>  Returns true if enters into listening mode else false <return>
    bool CreateSocket();

    /// <summary> Get Peer host </summary>
    /// <return>  Returns Device controller host <return>
    char *GetPeerIP() const;

    /// <summary> Sets device controller host </summary>
    /// <param name="pszHost"> Server host address to be set </param>
    /// <return>  Returns nothing <return>
    void SetPeerIP(const char *pszHost);

    /// <summary> Sets Peer port </summary>
    /// <param name="dwPort"> TCP port of the Peer</param>
    /// <return>  Returns nothing <return>
    void SetPeerPort(DWORD dwPort);


    /// <summary> Get Peer port </summary>
    /// <return>  Returns Peer port number <return>
    const DWORD GetPeerPort() const;

    /// <summary> Gets ICR port </summary>
    /// <return>  Returns ICR port number <return>
//    const DWORD GetICRPort() const;

    /// <summary> This method will send heartbeat message </summary>
    /// <return>  Returns true if message is sent successfully <return>
    void SendHeartBeatMessage();

    /// <summary> Setter for peerid </summary>
    /// <param name="dwpeerId"> peer id to set </param>
    /// <return>  Nothing <return>
    void            SetpeerId(DWORD dwpeerId);

    /// <summary> This method will return peerid </summary>
    /// <return>  Returns peerid <return>
    DWORD           GetpeerId() const
    {
        return m_dwpeerId;
    }
    
    /// <summary> This method will add 4 bytes header to the response </summary>
    /// <param name="pbtResponse"> Response prepared </param>
    /// <return>  Returns final response created <return>
    BYTE* PrepareFinalResponse(BYTE* pbtResponse);

    /// <summary> Getter for read message queue </summary>
    /// <return>  Returns read message queue <return>
    CMsgQueue*          GetReceiveQueue();

    /// <summary> Getter for send message queue </summary>
    /// <return>  Returns send message queue <return>
    CMsgQueue*          GetSendingQueue();

    bool SendMessage();
    bool ReadMessage();
    bool ProcessReceivedMessage();
    void SetLastHeartbeatReceived(DWORD dwReceivedTime);
    HANDLE GetShutdownEvent();
    void SetShutdownEvent(HANDLE hEvent);
    void EnableSSL(bool bEnablessl);
    void EnableVerifyCertificate(bool bVerifyCertificate);
    void SetSSLProperties(char* pcPassphrase, char* Certificate, char* PrivateKey);

protected:
    HANDLE      m_hThread;                  // Main thread handle
    HANDLE      m_hShutdownEvent;            // Main thread shutdown event
    CMsgQueue   m_receiveMessageQueue;      // Receive Message Queue
    CMsgQueue   m_sendingMessageQueue;      // Sending Message Queue

    /// <summary>Check if thread is started </summary>
    /// <return>  Returns true if started <return>
    bool IsStarted() const;

    /// <summary> It will close the connection</summary>
    /// <return>  Returns nothing <return>
    void CloseComm();        // close the socket

    /// <summary> It is the main function used to call send and recv function </summary>
    /// <return>  Returns nothing <return>
    void Run();

    /// <summary> Main thread function and will call run() function </summary>
    /// <param name="pParam"> Pointer of the class creating thread </param>
    /// <return>  Return status on completion <return>
    static UINT WINAPI SocketThreadProc(LPVOID pParam);

    /// <summary> This method will read 4 bytes header from the request </summary>
    /// <param name="pbtRequest"> Reuest received </param>
    /// <return>  Returns header read from the request <return>
    DWORD GetMessageLength(BYTE* pbtRequest);

    int ReadFullAmount(BYTE* pbtData, int iNoOfBytesToBeRead);

private:

    DWORD m_dwpeerId;
    CPeer* m_pPeer;//Back reference of the class containing CSocketManager
                               // object, so SocketManagert will not be responsible of deleting
                               // CPeer object.

    
    CTCPSocketNew *m_ConnectSocket;

    bool m_bIsOpen;
    bool m_bSSLEnabled;
    bool m_bVerifyCertificate;
    WCHAR* m_pwcPassphrase;
    WCHAR* m_pwcCertificate;
    WCHAR* m_pwcPrivateKey;
    DWORD m_dwLastHeartbeatReceivedTS;
    DWORD m_dwLastHeartbeatSentTS;
    DWORD m_dwPeerPort;
    char m_szPeerIP[256];
};

inline void CSocketManager::SetPeerIP(const char *pszHost)
{
    memcpy(&m_szPeerIP, pszHost, 256);
}

inline void CSocketManager::SetPeerPort(DWORD dwPort)
{
    m_dwPeerPort = dwPort;
}


inline char *CSocketManager::GetPeerIP() const
{
    return (char *)m_szPeerIP;
}

inline const DWORD CSocketManager::GetPeerPort() const
{
    return m_dwPeerPort;
}

inline CMsgQueue* CSocketManager::GetReceiveQueue()
{
    return &m_receiveMessageQueue;
}

inline CMsgQueue* CSocketManager::GetSendingQueue()
{
    return &m_sendingMessageQueue;
}

inline void CSocketManager::SetLastHeartbeatReceived(DWORD dwReceivedTime)
{
    m_dwLastHeartbeatReceivedTS = dwReceivedTime;
}

inline HANDLE CSocketManager::GetShutdownEvent()
{
    return m_hShutdownEvent;
}

inline void CSocketManager::SetShutdownEvent(HANDLE hEvent)
{
    m_hShutdownEvent = hEvent;
}
#endif


