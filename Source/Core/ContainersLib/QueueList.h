#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SystemLib/BasicTypes.h"

namespace Selas
{
    //=============================================================================================================================
    struct QueueListNode
    {
        void*          data;
        QueueListNode* nextNode;
    };

    //=============================================================================================================================
    struct QueueList
    {
        QueueList();
        ~QueueList();

        QueueListNode* front;
        QueueListNode* back;
    };

    void  QueueList_Initialize(QueueList* queueList, uint32 maxFreeListSize);
    void  QueueList_Shutdown(QueueList* queueList);

    bool  QueueList_Empty(QueueList* queueList);

    void  QueueList_Push(QueueList* queueList, void* data);
    void* QueueList_PopGeneric(QueueList* queueList);

    template <typename Type_>
    Type_ QueueList_Pop(QueueList* queueList)
    {
        return (Type_)QueueList_PopGeneric(queueList);
    }
}