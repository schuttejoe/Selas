//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/JsAssert.h>

#if AllowAsserts_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
namespace Shooty {

    void AssertHandler(const char* message, const char* filename, int line) {
        (void)filename;
        (void)line;
        (void)message;

        DebugBreak();
        //assert(0);
    }

}

#endif