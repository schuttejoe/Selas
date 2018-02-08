#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/Memory.h>
#include <SystemLib/JsAssert.h>

namespace Shooty {

    template <typename Type_>
    class CArray
    {
    public:
        CArray(void);
        ~CArray(void);

        void Close(void);
        void Clear(void);
        void Reserve(uint32 capacity);
        void Resize(uint32 length);

        const Type_* GetData(void) const { return m_data; }
        Type_* GetData(void) { return m_data; }

        inline Type_&       operator[] (uint index) { return m_data[index]; }
        inline const Type_& operator[] (uint index) const { return m_data[index]; }

        inline uint32 Length(void) const { return m_length; }
        inline uint32 Capacity(void) const { return m_capacity; }
        inline uint32 DataSize(void) const { return m_length * sizeof(Type_); }

        uint32 Add(void);
        uint32 Add(const Type_& element);
        void   Append(const CArray<Type_>& addend);

        bool Remove(const Type_& item);
        void RemoveFast(uint index);

    private:
        void ReallocateArray(uint32 newLength, uint32 newCapacity);
        void GrowArray(void);

    private:
        Type_ * m_data;
        uint32  m_length;
        uint32  m_capacity;
    };

    template<typename Type_>
    CArray<Type_>::CArray(void)
        : m_data(nullptr)
        , m_length(0)
        , m_capacity(0)
    {
    }

    template<typename Type_>
    CArray<Type_>::~CArray(void)
    {
        Close();
    }

    template<typename Type_>
    void CArray<Type_>::Close(void)
    {
        if(m_data) {
            Free_(m_data);
        }

        m_data = nullptr;
        m_length = 0;
        m_capacity = 0;
    }

    template<typename Type_>
    void CArray<Type_>::Clear(void)
    {
        m_length = 0;
    }

    template<typename Type_>
    void CArray<Type_>::Reserve(uint32 capacity)
    {
        if(capacity > m_capacity) {
            ReallocateArray(m_length, capacity);
        }
    }

    template<typename Type_>
    void CArray<Type_>::Resize(uint32 length)
    {
        if(length > m_capacity) {
            ReallocateArray(length, length);
        }
        else {
            m_length = length;
        }
    }

    template<typename Type_>
    uint32 CArray<Type_>::Add(void)
    {
        if(m_length == m_capacity) {
            GrowArray();
        }

        Assert_(m_length < m_capacity);
        return m_length++;
    }

    template<typename Type_>
    uint32 CArray<Type_>::Add(const Type_& element)
    {
        if(m_length == m_capacity) {
            GrowArray();
        }

        Assert_(m_length < m_capacity);
        m_data[m_length] = element;
        return m_length++;
    }

    template<typename Type_>
    void CArray<Type_>::Append(const CArray<Type_>& addend) {
        uint32 newLength = m_length + addend.Length();

        if (m_capacity < newLength)
            ReallocateArray(m_length, newLength);

        Memory::Copy(static_cast<Type_*>(m_data) + m_length, addend.GetData(), addend.DataSize());
        m_length = newLength;
    }

    template<typename Type_>
    bool CArray<Type_>::Remove(const Type_& item)
    {
        uint32 index = 0;
        for(; index < m_length; ++index) {
            if(m_data[index] == item) {
                break;
            }
        }

        if(index == m_length) {
            return false;
        }

        for(; index < m_length; ++index) {
            m_data[index] = m_data[index + 1];
        }

        --m_length;

        return true;
    }

    template<typename Type_>
    void CArray<Type_>::RemoveFast(uint index)
    {
        Assert_(index >= 0);
        Assert_(index < m_length);

        m_data[index] = m_data[m_length - 1];
        m_length--;
    }

    template<typename Type_>
    void CArray<Type_>::ReallocateArray(uint32 newLength, uint32 newCapacity)
    {
        Type_* newList = AllocArray_(Type_, newCapacity);

        if(m_data) {
            uint32 lengthToCopy = (m_length < newLength) ? m_length : newLength;
            if(lengthToCopy > 0) {
                Memory::Copy(newList, m_data, lengthToCopy * sizeof(Type_));
            }

            Free_(m_data);
        }

        m_data = newList;
        m_length = newLength;
        m_capacity = newCapacity;
    }

    template<typename Type_>
    void CArray<Type_>::GrowArray(void)
    {
        // Idea from BHG code; seems very sensible. Updated ranges though
        if(m_capacity < 64) {
            ReallocateArray(m_length, m_capacity + 16);
        }
        else if(m_capacity < 256) {
            ReallocateArray(m_length, m_capacity + 32);
        }
        else {
            ReallocateArray(m_length, m_capacity + 128);
        }
    }
}