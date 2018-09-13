#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SystemLib/BasicTypes.h"
#include "MathLib/FloatStructs.h"

namespace Selas
{
    enum SerializerFlags
    {
        eNone       = 0,
        eSerializerAttaching  = 0x01
    };

    //=============================================================================================================================
    class CSerializer
    {
    public:
        virtual SerializerFlags Flags() = 0;

        virtual void Serialize(void* data, uint size) = 0;
        virtual void SerializePtr(void*& data, uint size, uint alignment) = 0;
    };

    //=============================================================================================================================
    // -- Serialize functions for basic types
    void Serialize(CSerializer* serializer, bool& v);
    void Serialize(CSerializer* serializer, int8& v);
    void Serialize(CSerializer* serializer, int16& v);
    void Serialize(CSerializer* serializer, int32& v);
    void Serialize(CSerializer* serializer, int64& v);
    void Serialize(CSerializer* serializer, uint8& v);
    void Serialize(CSerializer* serializer, uint16& v);
    void Serialize(CSerializer* serializer, uint32& v);
    void Serialize(CSerializer* serializer, uint64& v);
    void Serialize(CSerializer* serializer, float& v);
    void Serialize(CSerializer* serializer, float2& v);
    void Serialize(CSerializer* serializer, float3& v);
    void Serialize(CSerializer* serializer, float4& v);
    void Serialize(CSerializer* serializer, float2x2& v);
    void Serialize(CSerializer* serializer, float3x3& v);
    void Serialize(CSerializer* serializer, float3x4& v);
    void Serialize(CSerializer* serializer, float4x4& v);
}