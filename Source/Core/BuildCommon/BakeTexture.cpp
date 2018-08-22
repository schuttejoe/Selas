//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/BakeTexture.h"
#include "BuildCore/BuildContext.h"
#include "TextureLib/TextureResource.h"
#include "StringLib/FixedString.h"

#include <stdio.h>

namespace Selas
{
    //=============================================================================================================================
    Error BakeTexture(BuildProcessorContext* context, TextureResourceData* data)
    {
        ReturnError_(context->CreateOutput(TextureResource::kDataType, TextureResource::kDataVersion, context->source.name.Ascii(),
                                           *data));

        return Success_;
    }
}