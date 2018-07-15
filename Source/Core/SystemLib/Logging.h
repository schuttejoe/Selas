#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SystemLib/BasicTypes.h"

namespace Selas
{
    namespace Logging
    {
        void WriteDebugInfo(const char* message, ...);
    }
}

#define EnableLogging_ 1

#if EnableLogging_

#define WriteDebugInfo_(info, ...) Logging::WriteDebugInfo(info, ##__VA_ARGS__)

#else

#define WriteDebugInfo_(info, ...)

#endif