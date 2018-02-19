#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

namespace Shooty {

    #define PlacementNew_(Type_, Var_)                     new(Var_) Type_()
    #define PlacementDelete_(Type_, Var_)                  (Var_)->~Type_()
                                                           
    #define New_(Type_)                                    ShootyNew<Type_>(__FUNCTION__, __FILE__, __LINE__)
    #define Delete_(Var_)                                  ShootyDelete(Var_)
    #define SafeDelete_(Var_)                              if (Var_) { ShootyDelete(Var_); Var_ = nullptr; }
                                                           
    #define Alloc_(AllocSize_)                             Shooty::ShootyMalloc(AllocSize_, __FUNCTION__, __FILE__, __LINE__)
    #define AllocArray_(Type_, Count_)                     static_cast<Type_*>(Shooty::ShootyMalloc(Count_ * sizeof(Type_), __FUNCTION__, __FILE__, __LINE__))
    #define Free_(Var_)                                    Shooty::ShootyFree(Var_)
    #define SafeFree_(Var_)                                if(Var_) { Shooty::ShootyFree(Var_); Var_ = nullptr; }

    #define AllocArrayAligned_(Type_, Count_, Alignment_)  static_cast<Type_*>(Shooty::ShootyAlignedMalloc(Count_ * sizeof(Type_), Alignment_, __FUNCTION__, __FILE__, __LINE__))
    #define AllocAligned_(AllocSize_, Alignment_)          Shooty::ShootyAlignedMalloc(AllocSize_, Alignment_, __FUNCTION__, __FILE__, __LINE__)
    #define FreeAligned_(Var_)                             Shooty::ShootyAlignedFree(Var_)
    #define SafeFreeAligned_(Var_)                         if(Var_) { Shooty::ShootyAlignedFree(Var_); Var_ = nullptr; }

    extern void* ShootyAlignedMalloc(size_t size, size_t alignment, const char* name, const char* file, int line);
    extern void* ShootyMalloc(size_t size, const char* name, const char* file, int line);
    extern void  ShootyAlignedFree(void* address);
    extern void  ShootyFree(void* address);

    //==============================================================================
    template <typename Type_>
    inline Type_* ShootyNew(const char* function, const char* file, int line) {
        Type_* mem = static_cast<Type_*>(ShootyMalloc(sizeof(Type_), function, file, line));
        PlacementNew_(Type_, mem);

        return mem;
    }

    //==============================================================================
    template <typename Type_>
    inline void ShootyDelete(Type_* memory) {
        PlacementDelete_(Type_, memory);
        ShootyFree(static_cast<void*>(memory));
    }
};