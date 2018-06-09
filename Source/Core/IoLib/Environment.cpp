//==============================================================================
// Joe Schutte
//==============================================================================

#include "IoLib/Environment.h"
#include "StringLib/StringUtil.h"

#include <stdio.h>

namespace Selas
{
    static FixedString128 rootDirectory;

    //==============================================================================
    void Environment_Initialize(cpointer projectName, cpointer exeDir)
    {
        FixedString256 sanitized;
        StringUtil::FullPathName(exeDir, sanitized.Ascii(), sanitized.Capcaity());

        char pathSep = StringUtil::PathSeperator();

        FixedString64 keyDir;
        #if IsWindows_
            sprintf_s(keyDir.Ascii(), keyDir.Capcaity(), "%s%c_BuildTemp", projectName, pathSep);
        #elif IsOsx_
            snprintf(keyDir.Ascii(), keyDir.Capcaity(), "%s%c_BuildTemp", projectName, pathSep);
        #endif

        // -- If we find this key then we're probably running from a dev environment.
        // -- Otherwise, we use the root
        cpointer root = StringUtil::FindSubString(sanitized.Ascii(), keyDir.Ascii());
        if(root) {
            uint32 toProjectDirSize = (uint32)(root - sanitized.Ascii());
            uint32 projectNameSize = StringUtil::Length(projectName);

            StringUtil::CopyN(rootDirectory.Ascii(), (int32)rootDirectory.Capcaity(), sanitized.Ascii(), (int32)(toProjectDirSize + projectNameSize + 1));
        }
        else {
            StringUtil::GetFolderPath(sanitized.Ascii(), rootDirectory.Ascii(), (uint32)rootDirectory.Capcaity());
        }
    }

    //==============================================================================
    FixedString128 Environment_Root()
    {
        return rootDirectory;
    }
}