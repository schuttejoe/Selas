//==============================================================================
// Joe Schutte
//==============================================================================

#include <IoLib/BinarySerializer.h>
#include <IoLib/Directory.h>
#include <SystemLib/JsAssert.h>
#include <stdio.h>

namespace Selas
{
    //==============================================================================
    Error SerializerStart(BinaryWriter* serializer, const char* filename, uint32 sizea, uint32 sizeb)
    {
        Directory::EnsureDirectoryExists(filename);

        serializer->file = nullptr;
        serializer->rawData.Close();
        serializer->pointers.Close();
        serializer->pointerData.Close();

        FILE* file = nullptr;
        fopen_s(&file, filename, "wb");
        if(file == nullptr) {
            return Error_("Failed to open file: %s", filename);
        }

        serializer->file = static_cast<void*>(file);

        if(sizea != 0) {
            serializer->rawData.Reserve(sizea);
        }
        if(sizeb != 0) {
            serializer->pointerData.Reserve(sizeb);
        }
        else if(sizea != 0) {
            serializer->pointerData.Reserve(sizea);
        }
        return Success_;
    }

    //==============================================================================
    Error SerializerEnd(BinaryWriter* serializer)
    {
        Assert_(serializer);
        Assert_(serializer->file != nullptr);
        Assert_(serializer->rawData.Length() + serializer->pointerData.Length() != 0); // Don't make zero-sized files

        // Fixup all of the pointers to their actual offsets
        uint64 pointer_data_offset = serializer->rawData.Length();
        for(uint scan = 0, count = serializer->pointers.Length(); scan < count; ++scan) {
            uint64 offset = pointer_data_offset + serializer->pointers[scan].pointerDataOffset;

            uint64* ptr = reinterpret_cast<uint64*>(&serializer->rawData[serializer->pointers[scan].pointerOffset]);
            *ptr = offset;
        }

        // Write the raw data
        fwrite(&serializer->rawData[0], 1, serializer->rawData.Length(), static_cast<FILE*>(serializer->file));
        fwrite(&serializer->pointerData[0], 1, serializer->pointerData.Length(), static_cast<FILE*>(serializer->file));

        int ret = fclose(static_cast<FILE*>(serializer->file));
        serializer->file = nullptr;

        if(ret != 0) {
            return Error_("Failed to close file with code: %d", ret);
        }

        return Success_;
    }

    //==============================================================================
    void SerializerWrite(BinaryWriter* serializer, const void* data, uint size)
    {
        Assert_(serializer);
        Assert_(serializer->file != nullptr);

        const uint8* cdata = static_cast<const uint8*>(data);
        for(uint scan = 0; scan < size; ++scan) {
            serializer->rawData.Add(cdata[scan]);
        }
    }

    //==============================================================================
    void SerializerWritePointerData(BinaryWriter* serializer, const void* data, uint size)
    {
        Assert_(serializer);
        Assert_(serializer->file != nullptr);

        const uint8* cdata = static_cast<const uint8*>(data);
        for(uint scan = 0; scan < size; ++scan) {
            serializer->pointerData.Add(cdata[scan]);
        }
    }

    //==============================================================================
    void SerializerWritePointerOffsetX64(BinaryWriter* serializer)
    {
        Assert_(serializer);
        Assert_(serializer->file != nullptr);

        PointerDesc desc;
        desc.pointerOffset = serializer->rawData.Length();
        desc.pointerDataOffset = serializer->pointerData.Length();
        serializer->pointers.Add(desc);

        uint64 null = 0;
        SerializerWrite(serializer, &null, sizeof(uint64));
    }

    //==============================================================================
    void SerializerStart(BinaryReader* serializer, void* data, uint32 size)
    {
        Assert_(serializer);

        serializer->data = data;
        serializer->size = size;
        serializer->offset = 0;
    }

    //==============================================================================
    void SerializerEnd(BinaryReader* serializer)
    {
        serializer->data = nullptr;
        serializer->size = 0;
        serializer->offset = 0;
    }

    //==============================================================================
    void SerializerRead(BinaryReader* serializer, void* data, uint32 size)
    {
        Assert_(serializer);
        Assert_(serializer->offset + size <= serializer->size);

        uint8* src = reinterpret_cast<uint8*>(serializer->data) + serializer->offset;
        Memory::Copy(data, src, size);

        serializer->offset += size;
    }

    //==============================================================================
    void SerializerAttach(BinaryReader* serializer, void** data, uint32 size)
    {
        Assert_(serializer);
        Assert_(serializer->offset + size <= serializer->size);

        uint8* src = reinterpret_cast<uint8*>(serializer->data) + serializer->offset;
        *data = src;

        serializer->offset += size;
    }
}