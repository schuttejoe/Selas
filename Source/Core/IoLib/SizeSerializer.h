#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "IoLib/Serializer.h"

namespace Selas
{
    class CSizeSerializer : public CSerializer
    {
    private:
        uint size = 0;

    public:

        virtual SerializerFlags Flags() override { return eNone; }

        virtual void Serialize(void* data, uint size) override;
        virtual void SerializePtr(void*& data, uint size, uint alignment) override;

        uint TotalSize();
    };
}