#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

namespace Shooty {

    #define PlacementNew_(VariableType_, Var_)          new(Var_) VariableType_()
    #define PlacementDelete_(VariableType_, Var_)       (Var_)->~VariableType_()

    #define New_(VariableType_)                         ShootyNew<VariableType_>(__FUNCTION__, __FILE__, __LINE__)
    #define Delete_(Var_)                               ShootyDelete(Var_)
    #define SafeDelete_(Var_)                           if (Var_) { ShootyDelete<VariableType_>(Var_); Var_ = nullptr; }

    #define Alloc_(AllocSize_)                          Shooty::ShootyMalloc(AllocSize_, __FUNCTION__, __FILE__, __LINE__)
    #define AllocArray_(VariableType_, Count_)          static_cast<VariableType_*>(Shooty::ShootyMalloc(Count_ * sizeof(VariableType_), __FUNCTION__, __FILE__, __LINE__))
    #define Free_(Var_)                                 Shooty::ShootyFree(Var_)
    #define AllocAligned_(AllocSize_, Alignment_)       Shooty::ShootyAlignedMalloc(AllocSize_, Alignment_, __FUNCTION__, __FILE__, __LINE__)
    #define FreeAligned_(Var_)                          Shooty::ShootyAlignedFree(Var_)

    extern void* ShootyAlignedMalloc(size_t size, size_t alignment, const char* name, const char* file, int line);
    extern void* ShootyMalloc(size_t size, const char* name, const char* file, int line);
    extern void  ShootyAlignedFree(void* address);
    extern void  ShootyFree(void* address);

    //==============================================================================
    template <typename VariableType_>
    inline VariableType_* ShootyNew(const char* function, const char* file, int line) {
        VariableType_* mem = static_cast<VariableType_*>(ShootyMalloc(sizeof(VariableType_), function, file, line));
        PlacementNew_(VariableType_, mem);

        return mem;
    }

    //==============================================================================
    template <typename VariableType_>
    inline void ShootyDelete(VariableType_* memory) {
        PlacementDelete_(VariableType_, memory);
        ShootyFree(static_cast<void*>(memory));
    }
};