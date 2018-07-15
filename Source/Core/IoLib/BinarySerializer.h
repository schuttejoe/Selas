#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "ContainersLib/CArray.h"
#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct PointerDesc
    {
        uint64 pointerOffset;
        uint64 pointerDataOffset;
    };

    //=============================================================================================================================
    struct BinaryWriter
    {
        CArray<uint8>       rawData;
        CArray<PointerDesc> pointers;
        CArray<uint8>       pointerData;
    };

    //=============================================================================================================================
    struct BinaryReader
    {
        void*   data;
        uint32  size;
        uint32  offset;
    };

    // Write interface
    void SerializerStart(BinaryWriter* serializer, uint32 sizea = 0, uint32 sizeb = 0);
    Error SerializerEnd(BinaryWriter* serializer, cpointer filepath);
    Error SerializerEnd(BinaryWriter* serializer, void*& data, uint32& dataSize);
    void SerializerWrite(BinaryWriter* serializer, const void* data, uint size);

    void SerializerWritePointerData(BinaryWriter* serializer, const void* data, uint size);
    void SerializerWritePointerOffsetX64(BinaryWriter* serializer);

    // Read interface
    void SerializerStart(BinaryReader* serializer, void* data, uint32 size);
    void SerializerEnd(BinaryReader* serializer);
    void SerializerRead(BinaryReader* serializer, void* data, uint32 size);
    void SerializerAttach(BinaryReader* serializer, void** data, uint32 size);

    //=============================================================================================================================
    template <typename T>
    inline void FixupPointerX64(void* base, T*& pointer)
    {
        uint64 offset = reinterpret_cast<uint64>(pointer);
        pointer = reinterpret_cast<T*>(reinterpret_cast<uint8*>(base) + offset);
    }
}