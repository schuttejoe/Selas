//==============================================================================
// Joe Schutte
//==============================================================================

#include <json/JsonDocument.h>
#include <IoLib/File.h>
#include <SystemLib/BasicTypes.h>
#include <SystemLib/MemoryAllocation.h>

// -- middleware
#include <rapidjson/document.h>

namespace Shooty
{
    struct JsonDocumentImpl
    {
        rapidjson::Document document;
    };

    //==============================================================================
    bool OpenJsonDocument(cpointer filepath, JsonDocument* document)
    {
        char* jsonString;
        uint32 jsonStringSize;

        ReturnFailure_(File::ReadWhileFileAsString(filepath, &jsonString, &jsonStringSize));
        document->impl = New_(JsonDocumentImpl);

        document->impl->document.Parse(jsonString);

        FreeAligned_(jsonString);

        return true;
    }
}