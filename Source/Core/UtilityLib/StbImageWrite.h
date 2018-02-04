//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty {

    enum StbImageFormats {
        PNG,
        BMP,
        TGA,
        HDR,
        JPG
    };

    bool StbImageWrite(cpointer filepath, uint width, uint height, StbImageFormats format, void* rgba);
}