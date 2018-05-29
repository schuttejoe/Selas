#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <StringLib/FixedString.h>
#include <SystemLib/BasicTypes.h>

namespace Selas
{
    struct Environment
    {
        FixedString256 engineRoot;
        FixedString256 assetsDir;
    };


    void Environment_Initialize(Environment* env, cpointer rootDirectoryName, cpointer exeDir);

    void Environment_RegisterAssetType(Environment* env, cpointer typeName);
}