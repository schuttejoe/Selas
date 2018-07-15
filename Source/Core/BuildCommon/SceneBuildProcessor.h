#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "BuildCore/BuildProcessor.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    class CSceneBuildProcessor : public CBuildProcessor
    {
        virtual Error    Setup() override;
        virtual cpointer Type() override;
        virtual uint64   Version() override;
        virtual Error    Process(BuildProcessorContext* context) override;
    };
}