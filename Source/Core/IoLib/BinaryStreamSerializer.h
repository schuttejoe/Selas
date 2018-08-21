#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "IoLib/SizeSerializer.h"
#include "IoLib/BinarySerializers.h"
#include "SystemLib/MemoryAllocation.h"

namespace Selas
{
    template<typename Type_>
    void SerializeToBinary(Type_& object, uint8*& data, uint& dataSize)
    {
        CSizeSerializer* sizeSerializer = New_(CSizeSerializer);
        Serialize(sizeSerializer, object);

        dataSize = sizeSerializer->TotalSize();
        data = AllocArrayAligned_(uint8, dataSize, 4096);
        Delete_(sizeSerializer);

        CBinaryWriteSerializer* writer = New_(CBinaryWriteSerializer);
        
        writer->Initialize(data, dataSize);
        Serialize(writer, object);
        writer->SwitchToPtrWrites();
        Serialize(writer, object);

        Delete_(writer);
    }

    template<typename Type_>
    void AttachToBinary(Type_*& object, uint8* data, uint dataSize)
    {
        CBinaryAttachSerializer* serializer = New_(CBinaryAttachSerializer);
        serializer->Initialize(data, dataSize);

        *reinterpret_cast<void**>(&object) = reinterpret_cast<Type_*>(data);
        Serialize(serializer, *object);

        Delete_(serializer);
    }
}