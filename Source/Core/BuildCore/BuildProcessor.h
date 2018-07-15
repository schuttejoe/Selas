#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "SystemLib/Error.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    struct BuildProcessorContext;

    //=============================================================================================================================
    class CBuildProcessor
    {
    public:
        virtual ~CBuildProcessor() { }

        virtual Error    Setup()                                 = 0;
        virtual cpointer Type()                                  = 0;
        virtual uint64   Version()                               = 0;
        virtual Error    Process(BuildProcessorContext* context) = 0;
    };
}
