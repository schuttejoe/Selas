//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/JsAssert.h>

#if AllowAsserts_

#if IsWindows_
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <assert.h>
#endif

namespace Selas
{
    void AssertHandler(const char* message, const char* filename, int line)
    {
        (void)filename;
        (void)line;
        (void)message;

        #if IsWindows_
        	DebugBreak();
        #else
        	assert(0);		
    	#endif
    }
}

#endif