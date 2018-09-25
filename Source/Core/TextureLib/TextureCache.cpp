//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "TextureLib/TextureCache.h"
#include "TextureLib/TextureFiltering.h"
#include "SystemLib/JsAssert.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/Atomic.h"

#include <map>

namespace Selas
{
    struct TextureMapEntry
    {
        volatile int64 usageRefCount;
        uint32 loadRefCount;        
        TextureResource resource;
    };

    typedef std::map<Hash32, TextureMapEntry*>           TextureResourceMap;
    typedef std::pair<Hash32, TextureMapEntry*>          TextureResourceKeyValue;
    typedef std::map<Hash32, TextureMapEntry*>::iterator TextureResourceIterator;

    struct TextureCacheData
    {
        TextureResourceMap map;
        uint64 capacity;

    };

    //=============================================================================================================================
    TextureCache::TextureCache()
        : cacheData(nullptr)
    {

    }

    //=============================================================================================================================
    TextureCache::~TextureCache()
    {
        Assert_(cacheData == nullptr);
    }

    //=============================================================================================================================
    void TextureCache::Initialize(uint64 cacheSize)
    {
        cacheData = New_(TextureCacheData);
        cacheData->capacity = cacheSize;
    }

    //=============================================================================================================================
    void TextureCache::Shutdown()
    {
        SafeDelete_(cacheData);
    }

    //=============================================================================================================================
    Error TextureCache::LoadTextureResource(const FilePathString& textureName, TextureHandle& handle)
    {
        TextureMapEntry* entry = New_(TextureMapEntry);
        Error err = ReadTextureResource(textureName.Ascii(), &entry->resource);
        if(Failed_(err)) {
            Delete_(entry);
            return err;
        }

        handle.hash = MurmurHash3_x86_32(textureName.Ascii(), StringUtil::Length(textureName.Ascii()));

        entry->loadRefCount = 1;
        entry->usageRefCount = 0;
        cacheData->map.insert(TextureResourceKeyValue(handle.hash, entry));

        return Success_;
    }

    //=============================================================================================================================
    //Error TextureCache::LoadTexturePtex(const FilePathString& filepath, TextureHandle& handle)
    //{
    //    TextureResource* resource = New_(TextureResource);
    //    Error err = ReadTextureResource(filepath.Ascii(), resource);
    //    if(Failed_(err)) {
    //        Delete_(resource);
    //        return err;
    //    }
    //    
    //    return Success_;
    //}

    //=============================================================================================================================
    void TextureCache::UnloadTexture(TextureHandle handle)
    {
        auto obj = cacheData->map.find(handle.hash);
        if(obj == cacheData->map.end()) {
            AssertMsg_(false, "Freeing texture that was never loaded or has already been unloaded.");
        }

        if(obj->second->usageRefCount != 0) {
            AssertMsg_(false, "Freeing texture with a non-zero reference count.");
        }

        --obj->second->loadRefCount;
        if(obj->second->loadRefCount == 0) {
            ShutdownTextureResource(&obj->second->resource);
        }

        Delete_(obj->second);
        cacheData->map.erase(obj);
    }

    //=============================================================================================================================
    const TextureResource* TextureCache::FetchTexture(TextureHandle handle)
    {
        if(handle.Valid() == false) {
            return nullptr;
        }

        auto obj = cacheData->map.find(handle.hash);
        if(obj == cacheData->map.end()) {
            AssertMsg_(false, "Attempting to fetch texture that was never loaded.");
            return nullptr;
        }

        Atomic::Increment64(&obj->second->usageRefCount);
        return &obj->second->resource;
    }

    //=============================================================================================================================
    void TextureCache::ReleaseTexture(TextureHandle handle)
    {
        if(handle.Valid() == false) {
            return;
        }

        auto obj = cacheData->map.find(handle.hash);
        if(obj == cacheData->map.end()) {
            AssertMsg_(false, "Attempting to fetch texture that was never loaded.");
            return;
        }

        Assert_(obj->second->usageRefCount != 0);
        Atomic::Decrement64(&obj->second->usageRefCount);
    }
}