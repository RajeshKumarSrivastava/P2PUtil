#include "stdafx.h"
#include "CSocketManager.h"
#include "Peer.h"


#define SEND_HEARTBEAT_TIMEOUT      1000000   // 10 seconds
#define RECEIVE_HEARTBEAT_TIMEOUT   150000   // 15 seconds
#define READ_TIMEOUT                300     // .3 second
#define WAIT_FOR_EVENT              200     // .2 second
#define HANDLE_COUNT                3       // No of event handles
#define BASE_PORT                   27000   // starting port number for ICR port
#define LOG_PREFIX                  40      // 40 is Added to accomodate timestamp + prefix (Data Rcvd:) during Logging

CSocketManager::CSocketManager(CPeer* pPeer):
    m_hThread(NULL),
    m_hShutdownEvent(NULL),
    m_receiveMessageQueue(),
    m_sendingMessageQueue(),
    m_dwpeerId(0),
    m_pPeer (pPeer),
    m_bIsOpen(false),
    m_dwLastHeartbeatReceivedTS(0),
    m_dwLastHeartbeatSentTS(0),
    m_dwPeerPort(0),
    m_ConnectSocket(NULL)
{
    memset(m_szPeerIP, 0, 256);
    std::cout <<"CSocketManager(CPeer*): Creating Socket Manager");
}

CSocketManager::~CSocketManager(void)
{
    // Terminate the socket connection/thread
    this->StopComm();
    if (this->m_ConnectSocket)
    {
        delete this->m_ConnectSocket;
        this->m_ConnectSocket = NULL;
    }

    if (m_pwcPassphrase)
    {
        delete [] m_pwcPassphrase;
    }
    if (m_pwcCertificate)
    {
        delete [] m_pwcCertificate;
    }
    if (m_pwcPrivateKey)
    {
        delete [] m_pwcPrivateKey;
    }
    
    // m_pPeer is back reference of the container class, so CSocketManager will
    // not be responsible of freeing m_pPeer object.
    m_pPeer = NULL;
    return;
}

DWORD CSocketManager::GetMessageLength(BYTE* pbtRequest)
{
    DWORD dwDataSize = 0;
    dwDataSize |= ((pbtRequest[0] & 0xFF) << 24);
    dwDataSize |= ((pbtRequest[1] & 0xFF) << 16);
    dwDataSize |= ((pbtRequest[2] & 0xFF) <<  8);
    dwDataSize |= ((pbtRequest[3] & 0xFF));

    return dwDataSize;
}

BYTE* CSocketManager::PrepareFinalResponse(BYTE* pbtResponse)
{
    DWORD dwLength = strlen(reinterpret_cast<char*>(pbtResponse));
    DWORD dwMsgBufferSize = dwLength + GENERIC_MSG_HEADER_SIZE;

    BYTE* pbtMsgBuffer = new BYTE[dwMsgBufferSize + 1];

    pbtMsgBuffer[0] = static_cast<BYTE>(dwLength >> 24 );  // MSB
    pbtMsgBuffer[1] = static_cast<BYTE>(dwLength >> 16 );  
    pbtMsgBuffer[2] = static_cast<BYTE>(dwLength >>  8 );  
    pbtMsgBuffer[3] = static_cast<BYTE>(dwLength       );  // LSB
    
    strncpy(reinterpret_cast<char*>(pbtMsgBuffer + GENERIC_MSG_HEADER_SIZE),
        reinterpret_cast<char*>(pbtResponse), dwLength);

    pbtMsgBuffer[dwMsgBufferSize] = '\0';
    return pbtMsgBuffer;
}

void CSocketManager::SendHeartBeatMessage()
{
    std::string strHeartBeatMessage;
    m_pPeer->CreateHeartBeatMessage(strHeartBeatMessage);

    BYTE* pbtMsgBuffer = this->PrepareFinalResponse(reinterpret_cast<BYTE*>
        (const_cast<char*>(strHeartBeatMessage.c_str())));

    m_sendingMessageQueue.AddMsgToQueue(pbtMsgBuffer);
    return;
}

void CSocketManager::SetpeerId(DWORD dwpeerId)
{
    m_dwpeerId = dwpeerId;
}

bool CSocketManager::IsOpen()
{
    return m_bIsOpen;
}

// just shut it all down
void CSocketManager::StopComm()
{
    // Close Socket
    if (this->IsOpen())
    {
        this->CloseComm();
    }

    // Kill Thread
    if (this->IsStarted())
    {
        TerminateThread(m_hThread, 1L);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
}

bool CSocketManager::CreateSocket()
{
    DWORD dwLogId = this->GetpeerId();
    DWORD localport = BASE_PORT;

    if (!m_ConnectSocket->ResolveRemoteAddress(GetPeerIP()))
    {
		std::cout << "Could not resolve DC host: "<< GetPeerIP()<<std::endl;
        return false;
    }
        
    // Try to connect
    m_ConnectSocket->Disconnect();

    if(!m_ConnectSocket->OpenSocket())
    {
        std::cout<<"Unable to open socket"<<std::endl;
    }

    // Bind the client to a port, this should be needed as the peer should have that list

    while(!m_ConnectSocket->Bind(localport))
    {
        std::cout<<"Unable to bind on port: "<<localport<<std::endl;
        // Attempt to bind to next possible port
        Sleep(100);
        localport += 100;        
    }

    if (localport > 0xFFFF)
    {
        return false;
    }

    m_ConnectSocket->SetRemotePort((USHORT)GetPeerPort());
    if (!m_ConnectSocket->Connect(2000))
    {
        std::cout <<"Unable to connect to Peer:"<<GetPeerIP()<<GetPeerPort()<<std::endl;
        char str[2046] = {0};
        sprintf(str, "%S", m_ConnectSocket->GetErrorMessage().c_str());
        std::cout<<"Lasterror :"<<str<<std::endl;
        return false;
    }   

    // We are successfully connected
    std::cout<<"Connected to Peer " << GetPeerIP() << GetPeerPort()<<localport;

    m_bIsOpen = true;
    return true;
}
bool CSocketManager::IsStarted() const
{
    return (NULL != m_hThread);
}

void CSocketManager::CloseComm()
{
    if (this->IsOpen())
    {
        this->m_ConnectSocket->Disconnect();
        m_bIsOpen = false;
    }

    // Clear the display, if ICR is disconnected or stopped
    // peer might not be valid when closing the Simpeer.exe
    if (m_pPeer->Getpeer() != NULL)
    {
        if (m_pPeer->Getpeer()->GetDisplay() != NULL)
        {
            m_pPeer->Getpeer()->GetDisplay()->Clear();
        }
    }
}

// This function will process the data added to reading queue
bool CSocketManager::ProcessReceivedMessage()
{
    DWORD dwLogId = this->GetpeerId();
    bool bResult = false;
    BYTE* pbtData = m_receiveMessageQueue.GetMessage();

    if (NULL != pbtData)
    {
        m_pPeer->ProcessICRRequest(reinterpret_cast<char*>(pbtData));

        delete[] pbtData;
        return true;
    }
    else
    {
        _InICRCmds(dwLogId, "Reading from queue failed");
        return false;
    }
}

int CSocketManager::ReadFullAmount(BYTE* pbtData, int iNoOfBytesToBeRead)
{
    int iAmountLeft = iNoOfBytesToBeRead;
    int iAmountRead = 0;

    while(iAmountLeft > 0)
    {
        iAmountRead = m_ConnectSocket->Read(pbtData + (iNoOfBytesToBeRead - iAmountLeft), iAmountLeft, READ_TIMEOUT);
        
        if (iAmountRead > 0)
        {
            iAmountLeft -= iAmountRead;
        }
        else
        {
            break;
        }
    }

    return iNoOfBytesToBeRead - iAmountLeft;
}

// This function will read the data over sockets and add the data received to 
// queue, if found or disconnect if any failure occurs
bool CSocketManager::ReadMessage()
{
    DWORD dwLogId = this->GetpeerId();
    bool bResult = false;

    BYTE btHeader[GENERIC_MSG_HEADER_SIZE] = {0};

    DWORD dwBytes = this->ReadFullAmount(btHeader, GENERIC_MSG_HEADER_SIZE);

    if (dwBytes == GENERIC_MSG_HEADER_SIZE)
    {
        // Data found over sockets
        DWORD dwMessageLength = this->GetMessageLength(btHeader);

        BYTE* pbtData = new BYTE[dwMessageLength + 1];

        DWORD dwBytes = this->ReadFullAmount(pbtData, dwMessageLength);

        if (dwMessageLength != dwBytes)
        {
            // Data is not received properly
            _OutICRCmdsf(dwLogId, "Unable to read the incoming message properly. "
                "Message read: %s", reinterpret_cast<char*>(pbtData));

            delete[] pbtData;
            bResult = false;
        }
        else
        {
            // All data has been received successfully
            bResult = true;
            pbtData[dwMessageLength] = '\0';
            DWORD dwLogLength = dwMessageLength + LOG_PREFIX;
            _InICRCmdsf(dwLogId, dwLogLength, "Data Received: %d%s", dwMessageLength, reinterpret_cast<char*>(pbtData));

            // Log message into c:\SimpeersCommunicationCurrent.log
            time_t     now = time(0);
            struct tm  tstruct;
            char       buf[80];
            tstruct = *localtime(&now);
            memset(buf, 0, sizeof(buf));
            strftime(buf, sizeof(buf), "%Y-%m-%d:%H:%M:%S", &tstruct);

            char* cData = new char[dwLogLength];
            memset(cData, 0, dwLogLength);
            sprintf(cData, "[%s]Data Rcvd: %d%s\r\n", buf, dwMessageLength, reinterpret_cast<char*>(pbtData));
            LogSocketCommunication(cData);
            m_receiveMessageQueue.AddMsgToQueue(pbtData);
            delete[] cData;
        }
    }
    else
    {
        // Check if socket is still connected
        if (!this->m_ConnectSocket->IsConnected())
        {
            // Some error has occur during Read, so socket has been disconnected
            _InICRCmds(dwLogId, "Connection closed.");
            bResult = false;
        }
        else
        {
            // Timeout occur
            bResult = true;
        }
    }

    return bResult;
}

// This function will extract the message from sending queue and send
// it over sockets
bool CSocketManager::SendMessage()
{   
    DWORD dwLogId = this->GetpeerId();
    bool bResult = false;
    BYTE* pbtData = m_sendingMessageQueue.GetMessage();

    if (NULL != pbtData)
    {
        // Data found on queue
        int iResponseLength = 0;
        iResponseLength = this->GetMessageLength(pbtData);
        DWORD dwNoOfBytesSent = this->m_ConnectSocket->Write(pbtData, (iResponseLength + GENERIC_MSG_HEADER_SIZE));

        if (dwNoOfBytesSent == (iResponseLength + GENERIC_MSG_HEADER_SIZE))
        {
             DWORD dwLogLength = dwNoOfBytesSent + LOG_PREFIX;
            // All the data has been sent successfully
            _OutICRCmdsf(dwLogId, dwLogLength , "Sent Data: %d%s", iResponseLength, (pbtData + GENERIC_MSG_HEADER_SIZE));

            // Log message into c:\SimpeersCommunicationCurrent.log
            time_t     now = time(0);
            struct tm  tstruct;
            char       buf[80];
            tstruct = *localtime(&now);
            memset(buf, 0, sizeof(buf));
            strftime(buf, sizeof(buf), "%Y-%m-%d:%H:%M:%S", &tstruct);

            char* cData = new char[dwLogLength];
            memset(cData, 0, dwLogLength);
            sprintf(cData, "[%s]Data Sent: %d%s\r\n",buf, dwNoOfBytesSent, reinterpret_cast<char*>(pbtData + GENERIC_MSG_HEADER_SIZE));
            LogSocketCommunication(cData);
            bResult = true;
            delete[] cData;
        }
        else
        {
            _OutICRCmdsf(dwLogId, "Sending data failed - Result Code %d", (int)dwNoOfBytesSent);
            bResult = false;
        }

        delete[] pbtData;
    }
    else
    {
        _OutICRCmds(dwLogId, "Retrieved invalid data(NULL) from writing queue");
        bResult = false;
    }

    return bResult;
}

// main function called by thread proc
void CSocketManager::Run()
{
    DWORD dwLogId = this->GetpeerId();

    try
    {
        bool bKeepRunning = true;
        while (bKeepRunning)
        {
            bool bIsSocketConnected = false;
            while (!bIsSocketConnected)
            {
                // Trying to create socket
                bIsSocketConnected = this->CreateSocket();
                if (!bIsSocketConnected)
                {
                    // Creation of socket failed
                    _Simulator(dwLogId, "Failed to create Socket. Retrying....");
                }
            }

            // Clear the sending queue when the connection is found
            this->GetSendingQueue()->Clear();

            m_dwLastHeartbeatReceivedTS = GetTickCount();
            m_dwLastHeartbeatSentTS = GetTickCount();

            // Send Heartbeat as soon as connection is made
            this->SendHeartBeatMessage();
            // Send PrinterStatus as soon as connection is made
            CSimPrinter* pSimPrinter = m_pPeer->Getpeer()->GetPrinter();
            CPeerPrinter* pGenericPrinter = dynamic_cast<CPeerPrinter*>(pSimPrinter);
            if (pGenericPrinter != NULL)
            {
                pGenericPrinter->PrinterStatus();
            }

            HANDLE hWaitHandles[HANDLE_COUNT] = {0};
            hWaitHandles[0] = m_hShutdownEvent;
            hWaitHandles[1] = m_sendingMessageQueue.GetMsgEvent();
            hWaitHandles[2] = m_receiveMessageQueue.GetMsgEvent();
            while (this->IsOpen())
            {
                m_pPeer->Getpeer()->Blink(false);

                // Check EMV processing timeout
                CPeerEMVModule* pEMV = m_pPeer->GetEMVModule();

                if ( pEMV && (!wcscmp( pEMV->GetModuleState(), EMV_MODULE_STATE_AMOUNT_SELECTION )
                        || !wcscmp(pEMV->GetModuleState(), EMV_MODULE_STATE_ENTER_PIN )
                        || !wcscmp(pEMV->GetModuleState(), EMV_MODULE_STATE_APPLICATION_SELECTION ) ))
                {
                    // Timeout 0 means unlimited time
                    if ( pEMV->GetEMVTimeout() > 0 && 
                        GetTickCount() - pEMV->GetEMVStartTime() > pEMV->GetEMVTimeout())
                    {
                        std::vector<std::string> vNullList;
                        pEMV->EmvCompletionDataEvent(vNullList, DENIED, CUSTOMER_ENTRY_TIMEOUT);
                    }
                }

                DWORD dwWaitResult = 
                    ::WaitForMultipleObjects(HANDLE_COUNT, hWaitHandles, false, WAIT_FOR_EVENT);
                switch (dwWaitResult)
                {
                case WAIT_TIMEOUT:      // Read data over socket
                    if (!this->ReadMessage())
                    {
                        // Failure message has been logged inside function itself
                        this->CloseComm();
                        continue;
                    }
                    break;

                case WAIT_OBJECT_0:          // Shutdown event
                    this->CloseComm();
                    this->m_ConnectSocket->CloseSocket();    // Close listening socket when ICR is stopped
                    bKeepRunning = false;
                    continue;

                case WAIT_OBJECT_0 + 1:      // Send data over socket from queue
                    if (!this->SendMessage())
                    {
                        // Failure message has been logged inside function itself
                        this->CloseComm();
                        continue;
                    }
                    break;
                case WAIT_OBJECT_0 + 2:  // Read data from queue
                    if (!this->ProcessReceivedMessage())
                    {
                        // Failure message has been logged inside function itself
                        this->CloseComm();
                        continue;
                    }
                    break;
                default:
                    _OutICRCmds(dwLogId, "Unexpected Error occured while waiting for read/write "
                        "event");
                    this->CloseComm();
                    continue;
                }

                // Close the connection if heart beat is not received within time limit
                if ((GetTickCount() - m_dwLastHeartbeatReceivedTS) > RECEIVE_HEARTBEAT_TIMEOUT)
                {
                    _InICRCmds(dwLogId, "Receive Timeout expired");
                    this->CloseComm();
                    continue;
                }

                // Check the heartbeat timer and send heartbeat message
                if ((GetTickCount() - m_dwLastHeartbeatSentTS) > SEND_HEARTBEAT_TIMEOUT) 
                {
                    this->SendHeartBeatMessage();
                    m_dwLastHeartbeatSentTS = GetTickCount();
                }
            }

            // wait for the reconnect
            Sleep(2000);
        }
        // ICR has been stopped so closing handle
        CloseHandle(m_hThread);
        m_hThread = NULL;
        


    }
    catch (...)  // Irrespective of condition will handle everything 
    {
        Log("Unexpected Error occured, closing ICR.\r\n"); // Log message in log file
        _OutICRCmds(dwLogId, "Unexpected Error occured, closing ICR.");
        this->CloseComm();

        // Close Simpeer
        CWnd * pParent = AfxGetApp()->GetMainWnd();
        pParent->PostMessage(WM_CLOSE);
    }
}

UINT WINAPI CSocketManager::SocketThreadProc(LPVOID pParam)
{
    CSocketManager* pThis = reinterpret_cast<CSocketManager*>( pParam );
    _ASSERTE( pThis != NULL );

    pThis->Run();

    return 1U;
} // end SocketThreadProc

// start the thread that get the work done
bool CSocketManager::CreateSocketThread()
{
    DWORD dwLogId = this->GetpeerId();

    if (m_bSSLEnabled)
    {
        m_ConnectSocket = new CSSLSocket;

        if (((CSSLSocket *)m_ConnectSocket)->Init(m_bVerifyCertificate, m_pwcCertificate, 0))
        {
            Log("ICR: SSL init success.\r\n"); // Log message in log file
            _InICRCmds(dwLogId, "SSL init success");
        }
        else
        {
            Log("ICR: SSL init unsuccessful.\r\n"); // Log message in log file
            _InICRCmds(dwLogId, "SSL init unsuccessful");
            return false;
        }
    }
    else
    {
        m_ConnectSocket = new CTCPSocketNew;
    }

    HANDLE hThread;
    UINT uiThreadId = 0;
    hThread = (HANDLE)_beginthreadex(NULL,    // Security attributes
        0,    // stack
        SocketThreadProc,    // Thread proc
        this,    // Thread param - used to call run on this instance
        CREATE_SUSPENDED,    // creation mode
        &uiThreadId);    // Thread ID

    if (NULL != hThread)
    {
        // creating a shutdown event for thread
        m_hShutdownEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
        ResumeThread( hThread );
        m_hThread = hThread;
        return true;
    }

    return false;
}

void CSocketManager::EnableSSL(bool bEnabled)
{
    m_bSSLEnabled = bEnabled;
}

void CSocketManager::EnableVerifyCertificate(bool bVerifyCertificate)
{
    m_bVerifyCertificate = bVerifyCertificate;
}

void CSocketManager::SetSSLProperties(char* pcPassphrase, char* pcCertificate, char* pcPrivateKey)
{
    if (strlen(pcPassphrase) > 0)
    {
        m_pwcPassphrase = new WCHAR[strlen(pcPassphrase)  + 1];
        mbstowcs(m_pwcPassphrase, pcPassphrase, strlen(pcPassphrase));
        m_pwcPassphrase[strlen(pcPassphrase)] = 0;
    }
    else
    {
        m_pwcPassphrase = nullptr;
    }

    m_pwcCertificate = new WCHAR[strlen(pcCertificate)  + 1];
    mbstowcs(m_pwcCertificate, pcCertificate, strlen(pcCertificate));
    m_pwcCertificate[strlen(pcCertificate)] = 0;

    m_pwcPrivateKey = new WCHAR[strlen(pcPrivateKey)  + 1];
    mbstowcs(m_pwcPrivateKey, pcPrivateKey, strlen(pcPrivateKey));
    m_pwcPrivateKey[strlen(pcPrivateKey)] = 0;
}
