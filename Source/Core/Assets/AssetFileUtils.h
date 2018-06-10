#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "UtilityLib/MurmurHash.h"
#include "StringLib/FixedString.h"
#include "SystemLib/BasicTypes.h"

namespace Selas
{
    #define InvalidAsset_ 0xFFFFFFFF

    struct AssetId
    {
        Hash32 type;
        Hash32 name;

        AssetId() : type(InvalidAsset_), name(InvalidAsset_) { }
        AssetId(cpointer type, cpointer name);
        AssetId(Hash32 type, Hash32 name);

        bool Valid() { return type != InvalidAsset_ && name != InvalidAsset_; }
    };

    //==============================================================================
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

    //==============================================================================
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
        void EnsureAssetDirectory(cpointer type);

        void AssetFilePath(cpointer type, uint32 version, cpointer name, FilePathString& filepath);
        void AssetFilePath(AssetId id, uint32 version, FilePathString& filepath);
    }
}