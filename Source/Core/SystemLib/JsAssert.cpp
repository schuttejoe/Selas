//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/JsAssert.h>

#if AllowAsserts_

#include <assert.h>

namespace Shooty {

    void AssertHandler(const char* message, const char* filename, int line) {
        (void)filename;
        (void)line;

        assert(message);//, filename, line);
    }

}

#endif