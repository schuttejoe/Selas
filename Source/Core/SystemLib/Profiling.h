#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemLib/BasicTypes.h"

namespace Selas
{
    #if USE_PIX
        class ScopedProfileEvent
        {
        public:
            ScopedProfileEvent(uint64 color, const char* name);
            ~ScopedProfileEvent();
        };

        #define ProfileEventMarker_(color, name) ScopedProfileEvent __profileEventMarker(color, name)
    #else
        #define ProfileEventMarker_(color, name)
    #endif
    
}