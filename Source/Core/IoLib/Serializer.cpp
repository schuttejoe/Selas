//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "IoLib/Serializer.h"

namespace Selas
{
    #define BasicTypeSerializerImpl_(Type_)                      \
        void Serialize(CSerializer* serializer, Type_ v)         \
        {                                                        \
            serializer->Serialize(&v, sizeof(v));                \
        }

    //=============================================================================================================================
    BasicTypeSerializerImpl_(int8);
    BasicTypeSerializerImpl_(int16);
    BasicTypeSerializerImpl_(int32);
    BasicTypeSerializerImpl_(int64);
    BasicTypeSerializerImpl_(uint8);
    BasicTypeSerializerImpl_(uint16);
    BasicTypeSerializerImpl_(uint32);
    BasicTypeSerializerImpl_(uint64);
    BasicTypeSerializerImpl_(float2);
    BasicTypeSerializerImpl_(float3);
    BasicTypeSerializerImpl_(float4);
    BasicTypeSerializerImpl_(float2x2);
    BasicTypeSerializerImpl_(float3x3);
    BasicTypeSerializerImpl_(float3x4);
    BasicTypeSerializerImpl_(float4x4);
}