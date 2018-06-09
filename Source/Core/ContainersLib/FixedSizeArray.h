#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemLib/JsAssert.h"
#include "SystemLib/MinMax.h"

namespace Selas
{

    #define Within_(num, minimum, maximum)       ((num >= minimum) && (num < maximum))
    #define AssertRange_(value,minimum,maximum)  Assert_(Within_(value,minimum,maximum))
    #define AssertValidRange_(x)                 AssertRange_(static_cast<uint32>(x), 0, m_length)

    template <class Type_, uint32 Capacity_>
    class CFixedSizeArray
    {
    public:
        inline  CFixedSizeArray(void) { m_length = 0; }

        void Close(void) { m_length = 0; }
        void Clear(void) { m_length = 0; }

        uint32 Add(Type_ object);
        uint32 Add(void);

        void   RemoveFast(uint32 index);

        uint32 Length() const { return m_length; }
        uint32 Capacity() const { return Capacity_; }

        Type_& operator [](int8 index) { AssertValidRange_(index); return m_list[index]; }
        Type_& operator [](uint8 index) { AssertValidRange_(index); return m_list[index]; }
        Type_& operator [](int16 index) { AssertValidRange_(index); return m_list[index]; }
        Type_& operator [](uint16 index) { AssertValidRange_(index); return m_list[index]; }
        Type_& operator [](int32 index) { AssertValidRange_(index); return m_list[index]; }
        Type_& operator [](uint32 index) { AssertValidRange_(index); return m_list[index]; }
        Type_& operator [](int64 index) { AssertValidRange_(index); return m_list[index]; }
        Type_& operator [](uint64 index) { AssertValidRange_(index); return m_list[index]; }

        const Type_& operator [](int8 index)   const { AssertValidRange_(index); return m_list[index]; }
        const Type_& operator [](uint8 index)  const { AssertValidRange_(index); return m_list[index]; }
        const Type_& operator [](int16 index)  const { AssertValidRange_(index); return m_list[index]; }
        const Type_& operator [](uint16 index) const { AssertValidRange_(index); return m_list[index]; }
        const Type_& operator [](int32 index)  const { AssertValidRange_(index); return m_list[index]; }
        const Type_& operator [](uint32 index) const { AssertValidRange_(index); return m_list[index]; }
        const Type_& operator [](int64 index)  const { AssertValidRange_(index); return m_list[index]; }
        const Type_& operator [](uint64 index) const { AssertValidRange_(index); return m_list[index]; }

        Type_* get_data(void) { return &(m_list[0]); }
        const Type_* get_data(void) const { return &(m_list[0]); }

    private:
        Type_   m_list[Capacity_];
        uint32  m_length;
    };

    //==============================================================================
    template <class Type_, uint32 Capacity_>
    uint32 CFixedSizeArray<Type_, Capacity_>::Add(Type_ object)
    {
        AssertRange_(m_length + 1, 0, Capacity_);
        m_list[m_length] = object;
        ++m_length;

        return m_length - 1;
    }

    //==============================================================================
    template <class Type_, uint32 Capacity_>
    uint32 CFixedSizeArray<Type_, Capacity_>::Add(void)
    {
        AssertRange_(m_length + 1, 0, Capacity_);
        ++m_length;
        return m_length - 1;
    }

    //==============================================================================
    template <class Type_, uint32 Capacity_>
    void CFixedSizeArray<Type_, Capacity_>::RemoveFast(uint32 index)
    {
        AssertValidRange_(index);
        m_list[index] = m_list[m_length - 1];
        --m_length;
    }
}



