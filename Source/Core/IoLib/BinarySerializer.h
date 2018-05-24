#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>
#include <ContainersLib/CArray.h>

namespace Selas
{
    struct PointerDesc
    {
        uint64 pointerOffset;
        uint64 pointerDataOffset;
    };

    //==============================================================================
    struct BinaryWriter
    {
        void*     file;

        CArray<uint8>       rawData;
        CArray<PointerDesc> pointers;
        CArray<uint8>       pointerData;
    };

    //==============================================================================
    struct BinaryReader
    {
        void*   data;
        uint32  size;
        uint32  offset;
    };

    // Write interface
    bool SerializerStart(BinaryWriter* serializer, const char* filename, uint32 sizea = 0, uint32 sizeb = 0);
    bool SerializerEnd(BinaryWriter* serializer);
    bool SerializerWrite(BinaryWriter* serializer, const void* data, uint size);

    bool SerializerWritePointerData(BinaryWriter* serializer, const void* data, uint size);
    bool SerializerWritePointerOffsetX64(BinaryWriter* serializer);

    // Read interface
    void SerializerStart(BinaryReader* serializer, void* data, uint32 size);
    bool SerializerEnd(BinaryReader* serializer);
    void SerializerRead(BinaryReader* serializer, void* data, uint32 size);
    void SerializerAttach(BinaryReader* serializer, void** data, uint32 size);

    //==============================================================================
    template <typename T>
    inline void FixupPointerX64(void* base, T*& pointer)
    {
        uint64 offset = reinterpret_cast<uint64>(pointer);
        pointer = reinterpret_cast<T*>(reinterpret_cast<uint8*>(base) + offset);
    }
}