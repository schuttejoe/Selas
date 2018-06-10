//==============================================================================
// Joe Schutte
//==============================================================================

#include "ContainersLib/QueueList.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/JsAssert.h"

namespace Selas
{
    //==============================================================================
    static QueueListNode* QueueList_AllocateNode(QueueList* queueList)
    {
        if(queueList->freeList != nullptr) {
            QueueListNode* node = queueList->freeList;
            QueueListNode* next = node->nextNode;
            queueList->freeList = next;
            --queueList->freeListCount;

            return node;
        }

        return New_(QueueListNode);
    }

    //==============================================================================
    static void QueueList_FreeNode(QueueList* __restrict queueList, QueueListNode* node)
    {
        if(queueList->freeListCount >= queueList->maxFreeListSize) {
            Delete_(node);
            return;
        }

        if(queueList->freeList == nullptr) {
            queueList->freeList = node;
            return;
        }

        node->data = nullptr;
        node->nextNode = queueList->freeList;
        ++queueList->freeListCount;
    }

    //==============================================================================
    QueueList::QueueList()
        : front(nullptr)
        , back(nullptr)
        , freeList(nullptr)
        , maxFreeListSize(0)
        , freeListCount(0)
    {

    }

    //==============================================================================
    QueueList::~QueueList()
    {
        Assert_(front == nullptr);
        Assert_(back == nullptr);
        Assert_(freeList == nullptr);
    }

    //==============================================================================
    void QueueList_Initialize(QueueList* __restrict queueList, uint32 maxFreeListSize)
    {
        queueList->front = nullptr;
        queueList->back = nullptr;
        queueList->maxFreeListSize = maxFreeListSize;
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

        node = queueList->freeList;
        while(node != nullptr) {
            QueueListNode* next = node->nextNode;
            Delete_(node);
            node = next;
        }

        queueList->front = nullptr;
        queueList->back = nullptr;
        queueList->freeList = nullptr;
    }

    //==============================================================================
    void QueueList_Push(QueueList* __restrict queueList, void* data)
    {
        QueueListNode* node = QueueList_AllocateNode(queueList);
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
    void QueueList_PushFront(QueueList* __restrict queueList, void* data)
    {
        QueueListNode* entry = QueueList_AllocateNode(queueList);
        entry->data = data;
        entry->nextNode = queueList->front;

        if(queueList->front == nullptr) {
            queueList->front = entry;
            queueList->back = entry;
        }
        else {
            queueList->front = entry;
        }
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

        QueueList_FreeNode(queueList, node);

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