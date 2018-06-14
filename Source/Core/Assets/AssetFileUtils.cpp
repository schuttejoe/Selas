
//==============================================================================
// Joe Schutte
//==============================================================================

#include "Assets/AssetFileUtils.h"
#include "StringLib/StringUtil.h"
#include "IoLib/Environment.h"
#include "IoLib/Directory.h"

#define PlatformIndependentPathSep_ '~'

#define AssetsDirectoryName_ "_Assets"
#define ContentDirectoryName_ "Content"

namespace Selas
{
    //==============================================================================
    ContentId::ContentId()
    {
        type.Clear();
        name.Clear();
    }

    //==============================================================================
    ContentId::ContentId(cpointer type_, cpointer name_)
    {
        type.Copy(type_);
        name.Copy(name_);

        // JSTODO -- Validation that name doesn't contain path separators other than
        // -- '~' and that it doesn't contain the root
    }

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
        void ContentDirectoryRoot(FilePathString& filepath)
        {
            // JSTODO - cache this

            FixedString128 root = Environment_Root();
            char ps = StringUtil::PathSeperator();

            StringUtil::Sprintf(filepath.Ascii(), filepath.Capcaity(), "%s%s%c", root.Ascii(), ContentDirectoryName_, ps);
        }

        //==============================================================================
        void AssetsDirectoryRoot(FilePathString& filepath)
        {
            // JSTODO - cache this

            FixedString128 root = Environment_Root();
            char ps = StringUtil::PathSeperator();

            StringUtil::Sprintf(filepath.Ascii(), filepath.Capcaity(), "%s%s%c", root.Ascii(), AssetsDirectoryName_, ps);
            StringUtil::ReplaceAll(filepath.Ascii(), PlatformIndependentPathSep_, ps);
        }

        //==============================================================================
        void ContentFilePath(cpointer name, FilePathString& filepath)
        {
            FixedString128 root = Environment_Root();
            char ps = StringUtil::PathSeperator();

            StringUtil::Sprintf(filepath.Ascii(), filepath.Capcaity(), "%s%s%c%s", root.Ascii(), ContentDirectoryName_, ps, name);
            StringUtil::ReplaceAll(filepath.Ascii(), PlatformIndependentPathSep_, ps);
        }

        //==============================================================================
        void ContentFilePath(cpointer prefix, cpointer name, cpointer postfix, FilePathString& filepath)
        {
            FixedString128 root = Environment_Root();
            char ps = StringUtil::PathSeperator();

            StringUtil::Sprintf(filepath.Ascii(), filepath.Capcaity(), "%s%s%c%s%s%s", root.Ascii(), ContentDirectoryName_, ps, prefix, name, postfix);
            StringUtil::ReplaceAll(filepath.Ascii(), PlatformIndependentPathSep_, ps);
        }

        //==============================================================================
        void SanitizeContentPath(cpointer filepath, FilePathString& sanitized)
        {
            FilePathString root;
            ContentDirectoryRoot(root);

            StringUtil::SanitizePath(root.Ascii(), PlatformIndependentPathSep_, filepath, sanitized.Ascii(), sanitized.Capcaity());
        }

        //==============================================================================
        void EnsureAssetDirectory(cpointer typeStr, uint64 version)
        {
            FixedString128 root = Environment_Root();
            char ps = StringUtil::PathSeperator();

            FilePathString directory;
            StringUtil::Sprintf(directory.Ascii(), directory.Capcaity(), "%s%s%c%s%c%llu%c", root.Ascii(), AssetsDirectoryName_, ps, typeStr, ps, version, ps);

            Directory::EnsureDirectoryExists(directory.Ascii());
        }

        //==============================================================================
        void AssetFilePath(cpointer typeStr, uint64 version, cpointer nameStr, FilePathString& filepath)
        {
            FixedString128 root = Environment_Root();
            char ps = StringUtil::PathSeperator();

            StringUtil::Sprintf(filepath.Ascii(), filepath.Capcaity(), "%s%s%c%s%c%llu%c%s.bin", root.Ascii(), AssetsDirectoryName_, ps, typeStr, ps, version, ps, nameStr);
        }

        //==============================================================================
        //void AssetFilePath(AssetId id, uint64 version, FilePathString& filepath)
        //{
        //    FixedString128 root = Environment_Root();
        //    char ps = StringUtil::PathSeperator();

        //    StringUtil::Sprintf(filepath.Ascii(), filepath.Capcaity(), "%s%s%c%u%c%llu%c%u.bin", root.Ascii(), AssetsDirectoryName_, ps, id.type, ps, version, ps, id.name);
        //}
    }
}