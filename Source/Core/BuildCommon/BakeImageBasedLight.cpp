//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/BakeImageBasedLight.h"
#include "BuildCore/BuildContext.h"
#include "SceneLib/ImageBasedLightResource.h"

namespace Selas
{
    //=============================================================================================================================
    Error BakeImageBasedLight(BuildProcessorContext* context, ImageBasedLightResourceData* data)
    {
        ReturnError_(context->CreateOutput(ImageBasedLightResource::kDataType, ImageBasedLightResource::kDataVersion,
                                           context->source.name.Ascii(), *data));

        return Success_;
    }
}