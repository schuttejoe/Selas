#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "IoLib/Serializer.h"

namespace Selas
{
    //=============================================================================================================================
    class CBinaryWriteSerializer : public CSerializer
    {
    private:
        enum BinaryWriterState
        {
            eWritingRaw,
            eWritingPtr
        };

        uint8* memory    = nullptr;
        uint memorySize = 0;
        uint offset = 0;
        uint ptrOffset = 0;
        BinaryWriterState state = eWritingRaw;

    public:

        ~CBinaryWriteSerializer();

        void Initialize(uint8* memory, uint memorySize);
        void SwitchToPtrWrites();

        virtual SerializerFlags Flags() override { return eNone; }

        virtual void Serialize(void* data, uint size) override;
        virtual void SerializePtr(void*& data, uint size, uint alignment) override;
    };

    //=============================================================================================================================
    class CBinaryReadSerializer : public CSerializer
    {
    private:
        uint8* rootAddr = nullptr;
        uint8* memory = nullptr;
        uint memorySize = 0;

        uint offset = 0;

    public:

        void Initialize(uint8* memory, uint memorySize);

        virtual SerializerFlags Flags() override { return eNone; }

        virtual void Serialize(void* data, uint size) override;
        virtual void SerializePtr(void*& data, uint size, uint alignment) override;
    };

    //=============================================================================================================================
    class CBinaryAttachSerializer : public CSerializer
    {
    private:
        uint8* rootAddr = nullptr;
        uint8* memory = nullptr;
        uint memorySize = 0;
        uint offset = 0;

    public:

        void Initialize(uint8* memory, uint memorySize);

        virtual SerializerFlags Flags() override { return eSerializerAttaching; }

        virtual void Serialize(void* data, uint size) override;
        virtual void SerializePtr(void*& data, uint size, uint alignment) override;
    };
}