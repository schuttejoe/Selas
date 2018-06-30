#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "ContainersLib/CArray.h"
#include "SystemLib/OSThreading.h"
#include "SystemLib/Memory.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    #define LocalBufferTemplateParams_ template<typename Type_, uint LocalBufferSize_, uint SoftLockSize_>
    #define LocalBufferClass_ CLocallyBufferedArray<Type_, LocalBufferSize_, SoftLockSize_>

    //==============================================================================
    LocalBufferTemplateParams_
    class CLocallyBufferedArray
    {
    private:
        CArray<Type_>* sharedBuffer;
        void* spinlock;

        Type_ localBuffer[LocalBufferSize_];
        uint localCount;

        void Flush();

    public:
        void Initialize(CArray<Type_>* sharedBuffer_, void* spinlock_);
        void Add(const Type_& data);

        void FlushImmediate();
    };

    //==============================================================================
    LocalBufferTemplateParams_
    void LocalBufferClass_::Flush()
    {
        uint start = sharedBuffer->Length();
        sharedBuffer->Resize((uint32)(start + localCount));

        Memory::Copy(sharedBuffer->GetData() + start, &localBuffer[0], localCount * sizeof(Type_));
        localCount = 0;
    }

    //==============================================================================
    LocalBufferTemplateParams_
    void LocalBufferClass_::Initialize(CArray<Type_>* sharedBuffer_, void* spinlock_)
    {
        sharedBuffer = sharedBuffer_;
        spinlock = spinlock_;
        localCount = 0;
    }

    //==============================================================================
    LocalBufferTemplateParams_
    void LocalBufferClass_::Add(const Type_& data)
    {
        if(localCount == LocalBufferSize_) {
            EnterSpinLock(spinlock);
            Flush();
            LeaveSpinLock(spinlock);
        }

        localBuffer[localCount] = data;
        ++localCount;

        if(localCount >= SoftLockSize_) {
            if(TryEnterSpinLock(spinlock)) {
                Flush();
                LeaveSpinLock(spinlock);
            }
        }
    }

    //==============================================================================
    LocalBufferTemplateParams_
    void LocalBufferClass_::FlushImmediate()
    {
        EnterSpinLock(spinlock);
        Flush();
        LeaveSpinLock(spinlock);
    }
}