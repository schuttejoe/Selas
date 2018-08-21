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

        void Serialize(void* data, uint size) override;
        void SerializePtr(void*& data, uint size, uint alignment) override;

        uint TotalSize();
    };
}