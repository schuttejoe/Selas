#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "UtilityLib/MurmurHash.h"
#include "StringLib/FixedString.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    #define InvalidAsset_ 0xFFFFFFFF
    #define PlatformIndependentPathSep_ '~'

    //=============================================================================================================================
    struct ContentId
    {
        FixedString32  type;
        FixedString256 name;

        ContentId();
        ContentId(cpointer type_, cpointer name_);
    };

    struct AssetId
    {
        Hash32 type;
        Hash32 name;

        AssetId() : type(InvalidAsset_), name(InvalidAsset_) { }
        AssetId(cpointer type, cpointer name);
        AssetId(Hash32 type, Hash32 name);

        bool Valid() { return type != InvalidAsset_ && name != InvalidAsset_; }
    };

    //=============================================================================================================================
    inline bool operator<(const AssetId& lhs, const  AssetId& rhs)
    {
        if(lhs.type != rhs.type) {
            return lhs.type < rhs.type;
        }
        if(lhs.name != rhs.name) {
            return lhs.name < rhs.name;
        }

        return false;
    }

    //=============================================================================================================================
    inline bool operator>(const AssetId& lhs, const AssetId& rhs)
    {
        if(lhs.type != rhs.type) {
            return lhs.type > rhs.type;
        }
        if(lhs.name != rhs.name) {
            return lhs.name > rhs.name;
        }

        return false;
    }

    namespace AssetFileUtils
    {
        void ContentDirectoryRoot(FilePathString& filepath);
        void AssetsDirectoryRoot(FilePathString& filepath);

        void ContentFilePath(cpointer name, FilePathString& filepath);
        void ContentFilePath(cpointer prefix, cpointer name, cpointer postfix, FilePathString& filepath);
        void SanitizeContentPath(cpointer filepath, FilePathString& sanitized);

        void AssetFilePath(cpointer type, uint64 version, cpointer name, FilePathString& filepath);
        //void AssetFilePath(AssetId id, uint64 version, FilePathString& filepath);

        void EnsureAssetDirectory(cpointer typeStr, uint64 version);

        template<typename Type_>
        void EnsureAssetDirectory()
        {
            EnsureAssetDirectory(Type_::kDataType, Type_::kDataVersion);
        }

        template<typename Type_>
        void IndependentPathSeperators(Type_& filepath)
        {
            StringUtil::ReplaceAll(filepath.Ascii(), '\\', PlatformIndependentPathSep_);
            StringUtil::ReplaceAll(filepath.Ascii(), '/', PlatformIndependentPathSep_);
        }
    }
}