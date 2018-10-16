#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "TextureLib/TextureResource.h"
#include "UtilityLib/MurmurHash.h"
#include "StringLib/FixedString.h"

#pragma warning(push)
#pragma warning(disable : 4996)
#include "ptex/Include/Ptexture.h"
#pragma warning(pop)

namespace Selas
{
    struct TextureCacheData;

    #define InvalidTextureHandle_ 0

    struct TextureHandle
    {
        TextureHandle() : hash(InvalidTextureHandle_) { }

        bool Valid() { return hash != InvalidTextureHandle_;  }
        bool Invalid() { return hash == InvalidTextureHandle_; }
        bool operator!=(const TextureHandle& rhs)
        {
            return hash != rhs.hash;
        }
    private:
        friend class TextureCache;
        Hash32 hash;
    };

    class TextureCache
    {
    private:
        TextureCacheData* cacheData;

    public:
         TextureCache();
        ~TextureCache();

        void Initialize(uint64 cacheSize);
        void Shutdown();

        Error LoadTextureResource(const FilePathString& textureName, TextureHandle& handle);
        Error LoadTexturePtex(const FilePathString& filepath, TextureHandle& handle);
        void UnloadTexture(TextureHandle handle);

        const TextureResource* FetchTexture(TextureHandle handle);
        Ptex::PtexTexture* FetchPtex(TextureHandle handle);
        void ReleaseTexture(TextureHandle handle);
   };
}