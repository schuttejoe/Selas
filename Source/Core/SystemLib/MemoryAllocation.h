#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

namespace Shooty {

    // JSTODO - New and Delete using the Shooty functions and placement new/delete so they get memory tracking

    extern void* ShootyAlignedMalloc(size_t size, size_t alignment, const char* name, const char* file, int line);
    extern void* ShootyMalloc(size_t size, const char* name, const char* file, int line);
    extern void  ShootyAlignedFree(void* address);
    extern void  ShootyFree(void* address);

};

// New_, Delete_, SafeDelete_

#define New_(VarType_)                 new VarType_
#define Delete_(Var_)                  delete Var_
#define SafeDelete_(Var_)              if (Var_) { delete Var_; Var_ = nullptr; }

// NewArray_, DeleteArray_

#define NewArray_(VarType_, VarCount_) new VarType_[VarCount_]
#define DeleteArray_(Var_)             delete [] Var_  
#define SafeDeleteArray_(Var_)         if (Var_) { delete [] Var_; Var_ = nullptr; }

// Alloc_, Free_

#define Alloc_(AllocSize_)                         Shooty::ShootyMalloc(AllocSize_, __FUNCTION__, __FILE__, __LINE__)
#define AllocArray_(Type_, Count_)                 static_cast<Type_*>(Shooty::ShootyMalloc(Count_ * sizeof(Type_), __FUNCTION__, __FILE__, __LINE__))
#define Free_(Var_)                                Shooty::ShootyFree(Var_)
#define AllocAligned_(AllocSize_, AllocAlignment_) Shooty::ShootyAlignedMalloc(AllocSize_, AllocAlignment_, __FUNCTION__, __FILE__, __LINE__)
#define FreeAligned_(Var_)                         Shooty::ShootyAlignedFree(Var_)
