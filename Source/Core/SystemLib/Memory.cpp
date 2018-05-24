//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/Memory.h>
#include <string.h>

namespace Selas
{
    //==============================================================================
    // Memory
    //==============================================================================

    void Memory::Zero(void* destPointer, uint size)
    {
        memset(destPointer, 0, size);
    }

    void Memory::Copy(void* destPointer, void const* source, uint size)
    {
        memcpy(destPointer, source, size);
    }

    void Memory::Set(void* destPointer, unsigned char pattern, uint size)
    {
        memset(destPointer, pattern, size);
    }

    int32 Memory::Compare(void const* lhs, void const* rhs, uint size)
    {
        return memcmp(lhs, rhs, size);
    }
}
