//==============================================================================
// Joe Schutte
//==============================================================================

#include <IoLib/Environment.h>
#include <IoLib/Directory.h>
#include <StringLib/StringUtil.h>

#include <stdio.h>

namespace Selas
{
    //==============================================================================
    void Environment_Initialize(Environment* env, cpointer rootDirectoryName, cpointer exeDir)
    {
        FixedString256 dir;
        dir.Copy(exeDir);

        int32 length = StringUtil::Length(dir.Ascii());
        while(length > 0) {
            const char* lastDir = StringUtil::LastFileOrFolderName(dir.Ascii());
            if(StringUtil::EqualsIgnoreCase(lastDir, rootDirectoryName)) {
                break;
            }

            StringUtil::RemoveLastFileOrFolder(dir.Ascii());
            length = StringUtil::Length(dir.Ascii());
        }

        if(length > 0) {
            env->engineRoot.Copy(dir.Ascii());
        }
        else {
            env->engineRoot.Copy(exeDir);
        }
        StringUtil::ReplaceAll(env->engineRoot.Ascii(), '\\', '/');

        sprintf_s(env->assetsDir.Ascii(), env->assetsDir.Capcaity(), "%s/Assets_", env->engineRoot.Ascii());
    }

    //==============================================================================
    void Environment_RegisterAssetType(Environment* env, cpointer typeName)
    {
        FixedString256 assetDir;
        sprintf_s(assetDir.Ascii(), assetDir.Capcaity(), "%s/%s/", env->assetsDir.Ascii(), typeName);

        Directory::EnsureDirectoryExists(assetDir.Ascii());
    }
}