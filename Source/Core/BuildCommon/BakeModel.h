#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SystemLib/Error.h"
#include "BuildCommon/ModelBuildPipeline.h"

namespace Selas
{
    struct BuildProcessorContext;

    //=============================================================================================================================
    Error BakeModel(BuildProcessorContext* context, const BuiltModel& model);
}