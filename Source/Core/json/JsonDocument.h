#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    struct JsonDocumentImpl;

    struct JsonDocument
    {
        JsonDocumentImpl* impl;
    };

    bool OpenJsonDocument(cpointer filepath, JsonDocument* document);
}