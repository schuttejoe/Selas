//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "IoLib/BinarySerializers.h"
#include "SystemLib/JsAssert.h"
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
    void CBinaryWriteSerializer::Serialize(const void* data, uint size_)
    {
        Assert_(offset + size_ <= memorySize);

        if(state == eWritingRaw) {
            Memory::Copy(memory + offset, data, size_);
        }
        offset += size_;
    }

    //=============================================================================================================================
    void CBinaryWriteSerializer::SerializePtr(const void* data, uint size_, uint alignment)
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
                
            uint64 alignedSize = alignment == 0 ? 0 : (size_ + alignment - 1) & ~(alignment - 1);
            if(alignedSize > 0) {
                Assert_(ptrOffset + alignedSize <= memorySize);
                Memory::Zero(memory + ptrOffset, alignedSize);
                ptrOffset += alignedSize;
            }
        }
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
    void CBinaryAttachSerializer::Serialize(const void* data, uint size_)
    {
        Unused_(data);
        offset += size_;
    }

    //=============================================================================================================================
    void CBinaryAttachSerializer::SerializePtr(const void* data, uint size_, uint alignment)
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