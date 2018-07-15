#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "StringLib/FixedString.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    void Environment_Initialize(cpointer projectName, cpointer exeDir);

    FixedString128 Environment_Root();
}