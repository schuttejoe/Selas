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
        FilePathString ptexFilePath;
        TextureResource resource;
    };

    typedef std::map<Hash32, TextureMapEntry*>           TextureResourceMap;
    typedef std::pair<Hash32, TextureMapEntry*>          TextureResourceKeyValue;
    typedef std::map<Hash32, TextureMapEntry*>::iterator TextureResourceIterator;

    struct TextureCacheData
    {
        TextureResourceMap map;
        uint64 capacity;

        Ptex::PtexCache* ptexCache;
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
        uint32 maxFiles = 128;

        cacheData = New_(TextureCacheData);
        cacheData->capacity = cacheSize;
        cacheData->ptexCache = Ptex::PtexCache::create(maxFiles, cacheSize, true, nullptr, nullptr);
    }

    //=============================================================================================================================
    void TextureCache::Shutdown()
    {
        if(cacheData) {
            cacheData->ptexCache->release();
        }
        SafeDelete_(cacheData);
    }

    //=============================================================================================================================
    Error TextureCache::LoadTextureResource(const FilePathString& textureName, TextureHandle& handle)
    {
        if(textureName.Length() == 0) {
            handle = TextureHandle();
            return Success_;
        }

        handle.hash = MurmurHash3_x86_32(textureName.Ascii(), StringUtil::Length(textureName.Ascii()));

        auto obj = cacheData->map.find(handle.hash);
        if(obj != cacheData->map.end()) {
            ++obj->second->loadRefCount;
            return Success_;
        }

        TextureMapEntry* entry = New_(TextureMapEntry);
        Error err = ReadTextureResource(textureName.Ascii(), &entry->resource);
        if(Failed_(err)) {
            Delete_(entry);
            return err;
        }

        entry->ptexFilePath.Clear();
        entry->loadRefCount = 1;
        entry->usageRefCount = 0;
        cacheData->map.insert(TextureResourceKeyValue(handle.hash, entry));

        return Success_;
    }

    //=============================================================================================================================
    Error TextureCache::LoadTexturePtex(const FilePathString& filepath, TextureHandle& handle)
    {
        if(filepath.Length() == 0) {
            handle = TextureHandle();
            return Success_;
        }

        if(File::Exists(filepath.Ascii()) == false) {
            return Error_("Ptex file %s does not exist", filepath.Ascii());
        }

        handle.hash = MurmurHash3_x86_32(filepath.Ascii(), (uint32)filepath.Length());

        auto obj = cacheData->map.find(handle.hash);
        if(obj != cacheData->map.end()) {
            ++obj->second->loadRefCount;
            return Success_;
        }

        TextureMapEntry* entry = New_(TextureMapEntry);
        entry->ptexFilePath.Copy(filepath.Ascii());
        entry->loadRefCount = 1;
        entry->usageRefCount = 0;

        cacheData->map.insert(TextureResourceKeyValue(handle.hash, entry));

        return Success_;
    }

    //=============================================================================================================================
    void TextureCache::UnloadTexture(TextureHandle handle)
    {
        auto obj = cacheData->map.find(handle.hash);
        if(obj == cacheData->map.end()) {
            AssertMsg_(false, "Freeing texture that was never loaded or has already been unloaded.");
            return;
        }

        if(obj->second->usageRefCount != 0) {
            AssertMsg_(false, "Freeing texture with a non-zero reference count.");
        }

        --obj->second->loadRefCount;
        if(obj->second->loadRefCount == 0) {
            ShutdownTextureResource(&obj->second->resource);

            Delete_(obj->second);
            cacheData->map.erase(obj);
        }
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
    Ptex::PtexTexture* TextureCache::FetchPtex(TextureHandle handle)
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

        Ptex::String error;
        Ptex::PtexTexture* texture = cacheData->ptexCache->get(obj->second->ptexFilePath.Ascii(), error);
        Assert_(texture != nullptr);

        return texture;
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