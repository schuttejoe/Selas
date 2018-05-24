//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Selas
{
    static_assert(sizeof(int8) == 1, "Unexpected primitive size");
    static_assert(sizeof(int16) == 2, "Unexpected primitive size");
    static_assert(sizeof(int32) == 4, "Unexpected primitive size");
    static_assert(sizeof(int64) == 8, "Unexpected primitive size");

    static_assert(sizeof(uint8) == 1, "Unexpected primitive size");
    static_assert(sizeof(uint16) == 2, "Unexpected primitive size");
    static_assert(sizeof(uint32) == 4, "Unexpected primitive size");
    static_assert(sizeof(uint64) == 8, "Unexpected primitive size");
}