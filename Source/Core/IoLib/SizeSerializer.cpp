//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "IoLib/SizeSerializer.h"
#include "SystemLib/JsAssert.h"

namespace Selas
{
    //=============================================================================================================================
    void CSizeSerializer::Serialize(const void* data, uint size_)
    {
        Unused_(data);

        size += size_;
    }

    //=============================================================================================================================
    void CSizeSerializer::SerializePtr(const void* data, uint size_, uint alignment)
    {
        AssertMsg_((alignment & (alignment - 1)) == 0, "Alignment must be a power of two");

        Unused_(data);

        size += sizeof(void*);
        size += size_;
        size += (size_ + alignment - 1) & ~(alignment - 1);
    }

    //=============================================================================================================================
    uint CSizeSerializer::TotalSize()
    {
        return size;
    }
}