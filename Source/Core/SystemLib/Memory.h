#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemLib/BasicTypes.h"

namespace Selas
{
    //==============================================================================
    // Memory
    //==============================================================================

    namespace Memory
    {
        void Zero(void* destPointer, uint memorySize);
        void Copy(void* destPointer, void const* source_pointer, uint memorySize);
        void Set(void* destPointer, unsigned char memory_pattern, uint memorySize);
        int32 Compare(void const* lhs, void const* rhs, uint memorySize);
    };
}