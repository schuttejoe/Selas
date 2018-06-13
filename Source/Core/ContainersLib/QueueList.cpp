//==============================================================================
// Joe Schutte
//==============================================================================

#include "ContainersLib/QueueList.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/JsAssert.h"

namespace Selas
{
    //==============================================================================
    QueueList::QueueList()
        : front(nullptr)
        , back(nullptr)
    {

    }

    //==============================================================================
    QueueList::~QueueList()
    {
        Assert_(front == nullptr);
        Assert_(back == nullptr);
    }

    //==============================================================================
    void QueueList_Initialize(QueueList* __restrict queueList, uint32 maxFreeListSize)
    {
        queueList->front = nullptr;
        queueList->back = nullptr;
    }

    //==============================================================================
    void QueueList_Shutdown(QueueList* __restrict queueList)
    {
        QueueListNode* node = queueList->front;
        AssertMsg_(node == nullptr, "QueueList being shutdown while it still has valid entries");
        while(node != nullptr) {
            
            QueueListNode* next = node->nextNode;
            Delete_(node);
            node = next;
        }

        queueList->front = nullptr;
        queueList->back = nullptr;
    }

    //==============================================================================
    bool QueueList_Empty(QueueList* queueList)
    {
        return queueList->front == nullptr;
    }

    //==============================================================================
    void QueueList_Push(QueueList* __restrict queueList, void* data)
    {
        QueueListNode* node = New_(QueueListNode);
        node->data = data;
        node->nextNode = nullptr;

        if(queueList->back == nullptr) {
            queueList->front = node;
            queueList->back = node;
            return;
        }

        queueList->back->nextNode = node;
        queueList->back = node;
    }

    //==============================================================================
    void* QueueList_PopGeneric(QueueList* __restrict queueList)
    {
        if(queueList->front == nullptr) {
            return nullptr;
        }

        QueueListNode* node = queueList->front;
        void* data = node->data;
        QueueListNode* next = node->nextNode;

        Delete_(node);

        if(queueList->front == queueList->back) {
            queueList->front = nullptr;
            queueList->back = nullptr;
        }
        else {
            queueList->front = next;
        }

        return data;
    }

    //==============================================================================
    bool QueueList_IsEmpty(QueueList* __restrict queueList)
    {
        if(queueList->front == nullptr) {
            return true;
        }

        return false;
    }
}