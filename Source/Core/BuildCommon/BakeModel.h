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
    Error BakeModel(BuildProcessorContext* context, cpointer name, const BuiltModel& model);
}