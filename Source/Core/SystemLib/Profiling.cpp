//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemLib/Profiling.h"

#if USE_PIX
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <WinPixEventRuntime/pix3.h>
#endif

namespace Selas
{
#if USE_PIX
    //==============================================================================
    ScopedProfileEvent::ScopedProfileEvent(uint64 color, const char* name)
    {
        PIXBeginEvent(color, name);
    }

    //==============================================================================
    ScopedProfileEvent::~ScopedProfileEvent()
    {
        PIXEndEvent();
    }
#else

#endif
}
