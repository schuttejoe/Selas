//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "IoLib/BinarySerializers.h"
#include "SystemLib/JsAssert.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/Memory.h"

namespace Selas
{
    static const uint64 kTempPointerKey = 0x1234567812345678;
    static_assert(sizeof(void*) == sizeof(kTempPointerKey), "Expected pointer size of 64 bits");

    //=============================================================================================================================
    CBinaryWriteSerializer::~CBinaryWriteSerializer()
    {
        Assert_(ptrOffset == memorySize);
    }

    //=============================================================================================================================
    void CBinaryWriteSerializer::Initialize(uint8* memory_, uint memorySize_)
    {
        memory = memory_;
        memorySize = memorySize_;
        offset = 0;
        state = eWritingRaw;
    }

    //=============================================================================================================================
    void CBinaryWriteSerializer::SwitchToPtrWrites()
    {
        state = eWritingPtr;
        ptrOffset = offset;
        offset = 0;
    }

    //=============================================================================================================================
    void CBinaryWriteSerializer::Serialize(void* data, uint size_)
    {
        Assert_(offset + size_ <= memorySize);

        if(state == eWritingRaw) {
            Memory::Copy(memory + offset, data, size_);
        }
        offset += size_;
    }

    //=============================================================================================================================
    void CBinaryWriteSerializer::SerializePtr(void*& data, uint size_, uint alignment)
    {
        AssertMsg_(alignment == 0 || (alignment & (alignment - 1)) == 0, "Alignment must be a power of two");
        Assert_(offset + sizeof(ptrOffset) <= memorySize);

        if(state == eWritingRaw) {
            Memory::Copy(memory + offset, &kTempPointerKey, sizeof(kTempPointerKey));
            offset += sizeof(kTempPointerKey);
        }
        else {
            Assert_(ptrOffset + size_ <= memorySize);
            Assert_(*(uint64*)(memory + offset) == kTempPointerKey);

            Memory::Copy(memory + offset, &ptrOffset, sizeof(ptrOffset));
            offset += sizeof(ptrOffset);

            Memory::Copy(memory + ptrOffset, data, size_);
            ptrOffset += size_;
                
            uint64 alignedSize = alignment == 0 ? 0 : ((size_ + alignment - 1) & ~(alignment - 1)) - size_;
            if(alignedSize > 0) {
                Assert_(ptrOffset + alignedSize <= memorySize);
                Memory::Zero(memory + ptrOffset, alignedSize);
                ptrOffset += alignedSize;
            }
        }
    }

    //=============================================================================================================================
    void CBinaryReadSerializer::Initialize(uint8* memory_, uint memorySize_)
    {
        rootAddr = memory_;
        memory = memory_;
        memorySize_ = memorySize_;
        offset = 0;
    }

    //=============================================================================================================================
    void CBinaryReadSerializer::Serialize(void* data, uint size_)
    {
        Memory::Copy(data, memory + offset, size_);
        offset += size_;
    }

    //=============================================================================================================================
    void CBinaryReadSerializer::SerializePtr(void*& data, uint size_, uint alignment)
    {
        uint64 ptrOffset = *(uint64*)(memory + offset);
        uint64* address = reinterpret_cast<uint64*>(reinterpret_cast<uint8*>(rootAddr) + ptrOffset);

        uint8* dataAddr = alignment == 0 ? AllocArray_(uint8, size_) : AllocArrayAligned_(uint8, size_, alignment);

        Memory::Copy(dataAddr, address, size_);
        // -- only advance by the size of the pointer
        offset += sizeof(address);

        data = (void*&)dataAddr;
    }

    //=============================================================================================================================
    void CBinaryAttachSerializer::Initialize(uint8* memory_, uint memorySize_)
    {
        rootAddr = memory_;
        memory = memory_;
        memorySize_ = memorySize_;
        offset = 0;
    }

    //=============================================================================================================================
    void CBinaryAttachSerializer::Serialize(void* data, uint size_)
    {
        Unused_(data);
        offset += size_;
    }

    //=============================================================================================================================
    void CBinaryAttachSerializer::SerializePtr(void*& data, uint size_, uint alignment)
    {
        Unused_(data);
        Unused_(size_);
               
        uint64 ptrOffset = *(uint64*)(memory + offset);
        uint64* address = reinterpret_cast<uint64*>(reinterpret_cast<uint8*>(rootAddr) + ptrOffset);
        Memory::Copy(memory + offset, &address, sizeof(address));

        Assert_(alignment == 0 || ((uint)address & (alignment - 1)) == 0);

        offset += sizeof(address);
    }
}