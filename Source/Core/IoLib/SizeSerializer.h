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

        void Serialize(const void* data, uint size) override;
        void SerializePtr(const void* data, uint size, uint alignment) override;

        uint TotalSize();
    };
}