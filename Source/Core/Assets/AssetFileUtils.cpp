
//==============================================================================
// Joe Schutte
//==============================================================================

#include "Assets/AssetFileUtils.h"
#include "StringLib/StringUtil.h"
#include "IoLib/Environment.h"
#include "IoLib/Directory.h"

namespace Selas
{
    //==============================================================================
    AssetId::AssetId(cpointer typeStr, cpointer nameStr)
    {
        type = MurmurHash3_x86_32(typeStr, StringUtil::Length(typeStr), 0);
        name = MurmurHash3_x86_32(nameStr, StringUtil::Length(nameStr), 0);
    }

    //==============================================================================
    AssetId::AssetId(Hash32 type_, Hash32 name_)
        : type(type_)
        , name(name_)
    {

    }

    namespace AssetFileUtils
    {
        //==============================================================================
        void EnsureAssetDirectory(cpointer typeStr)
        {
            FixedString128 root = Environment_Root();
            char ps = StringUtil::PathSeperator();

            Hash32 type = MurmurHash3_x86_32(typeStr, StringUtil::Length(typeStr), 0);

            FilePathString directory;
            StringUtil::Sprintf(directory.Ascii(), directory.Capcaity(), "%s%cAssets_%c%u%c", root.Ascii(), ps, ps, type, ps);

            Directory::EnsureDirectoryExists(directory.Ascii());
        }

        //==============================================================================
        void AssetFilePath(cpointer typeStr, uint32 version, cpointer nameStr, FilePathString& filepath)
        {
            AssetFilePath(AssetId(typeStr, nameStr), version, filepath);
        }

        //==============================================================================
        void AssetFilePath(AssetId id, uint32 version, FilePathString& filepath)
        {
            FixedString128 root = Environment_Root();
            char ps = StringUtil::PathSeperator();

            StringUtil::Sprintf(filepath.Ascii(), filepath.Capcaity(), "%s%cAssets_%c%u%c%u%c%u.bin", root.Ascii(), ps, ps, id.type, ps, version, ps, id.name);
        }
    }
}