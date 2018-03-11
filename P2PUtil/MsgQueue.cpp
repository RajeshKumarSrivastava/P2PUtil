#include "StdAfx.h"
#include "MsgQueue.h"

CMsgQueue::CMsgQueue()
:m_csQueue()
{
    m_hMsgEvent = CreateEvent(NULL,  TRUE, FALSE, NULL);
}

CMsgQueue::~CMsgQueue()
{
    Clear(); 
    CloseHandle(m_hMsgEvent);
}

HANDLE CMsgQueue::GetMsgEvent()
{
    return m_hMsgEvent;
}

void CMsgQueue::AddMsgToQueue(BYTE* pbtBytes)
{
    CLock oQueueLocker(&m_csQueue);

    push(pbtBytes);
    SetEvent(m_hMsgEvent);
}

BYTE* CMsgQueue::GetMessage()
{
    BYTE* pbtBytes = NULL;
    CLock oQueueLocker(&m_csQueue);
 
    if (size())
    {
        pbtBytes = front();
        pop();
        if (size() == 0)
        {
            ResetEvent(m_hMsgEvent);
        }
    }
    
    return pbtBytes;
}

void CMsgQueue::Clear()
{
    CLock oQueueLocker(&m_csQueue);
    
    while (!empty())
    {
        BYTE *pbtMsg = front();
        delete pbtMsg;
        pop();
    }

    ResetEvent(m_hMsgEvent);
}

unsigned int CMsgQueue::Count()
{
    CLock oQueueLocker(&m_csQueue);
    int iSize = static_cast<unsigned int>(size());
    return iSize;
}

