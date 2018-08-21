//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCommon/BakeImageBasedLight.h"
#include "BuildCore/BuildContext.h"
#include "SceneLib/ImageBasedLightResource.h"
#include "MathLib/ImportanceSampling.h"
#include "IoLib/BinarySerializer.h"

namespace Selas
{
    //=============================================================================================================================
    Error BakeImageBasedLight(BuildProcessorContext* context, const ImageBasedLightResourceData* data)
    {
        ReturnError_(context->CreateOutput(ImageBasedLightResource::kDataType, ImageBasedLightResource::kDataVersion,
                                           context->source.name.Ascii(), *data));

        return Success_;
    }
}