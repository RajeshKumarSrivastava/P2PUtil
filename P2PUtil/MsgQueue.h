#ifndef __INC_MSG_QUEUE_H_
#define __INC_MSG_QUEUE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include <queue>

using std::queue;

class CMsgQueue : private queue<BYTE*>
{
  public:
    CMsgQueue();
    virtual ~CMsgQueue();

    void        AddMsgToQueue(BYTE* pbtBytes);
    BYTE* GetMessage();
    HANDLE GetMsgEvent();

    void Clear();
    unsigned int Count();

  private:
    void Lock()    { m_csQueue.Lock(); }
    void Unlock()  { m_csQueue.Unlock(); }

    CCriticalSection m_csQueue;
    HANDLE m_hMsgEvent;
};

#endif   //__INC_MSG_QUEUE_H_

