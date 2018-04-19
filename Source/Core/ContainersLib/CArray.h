#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/MemoryAllocation.h>
#include <SystemLib/Memory.h>
#include <SystemLib/JsAssert.h>

namespace Shooty
{

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

        const Type_* GetData(void) const { return _data; }
        Type_* GetData(void) { return _data; }

        inline Type_&       operator[] (uint index) { return _data[index]; }
        inline const Type_& operator[] (uint index) const { return _data[index]; }

        // -- JSTOO -- Rename to Count
        inline uint32 Length(void) const { return _count; }
        inline uint32 Capacity(void) const { return _capacity; }
        inline uint32 DataSize(void) const { return _count * sizeof(Type_); }

        uint32 Add(void);
        uint32 Add(const Type_& element);
        void   Append(const CArray<Type_>& addend);

        bool Remove(const Type_& item);
        void RemoveFast(uint index);

    private:
        void ReallocateArray(uint32 newLength, uint32 newCapacity);
        void GrowArray(void);

    private:
        Type_ * _data;
        uint32  _count;
        uint32  _capacity;
    };

    template<typename Type_>
    CArray<Type_>::CArray(void)
        : _data(nullptr)
        , _count(0)
        , _capacity(0)
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
        if(_data) {
            Free_(_data);
        }

        _data = nullptr;
        _count = 0;
        _capacity = 0;
    }

    template<typename Type_>
    void CArray<Type_>::Clear(void)
    {
        _count = 0;
    }

    template<typename Type_>
    void CArray<Type_>::Reserve(uint32 capacity)
    {
        if(capacity > _capacity) {
            ReallocateArray(_count, capacity);
        }
    }

    template<typename Type_>
    void CArray<Type_>::Resize(uint32 length)
    {
        if(length > _capacity) {
            ReallocateArray(length, length);
        }
        else {
            _count = length;
        }
    }

    template<typename Type_>
    uint32 CArray<Type_>::Add(void)
    {
        if(_count == _capacity) {
            GrowArray();
        }

        Assert_(_count < _capacity);
        return _count++;
    }

    template<typename Type_>
    uint32 CArray<Type_>::Add(const Type_& element)
    {
        if(_count == _capacity) {
            GrowArray();
        }

        Assert_(_count < _capacity);
        _data[_count] = element;
        return _count++;
    }

    template<typename Type_>
    void CArray<Type_>::Append(const CArray<Type_>& addend)
    {
        uint32 newLength = _count + addend.Length();

        if(_capacity < newLength)
            ReallocateArray(_count, newLength);

        Memory::Copy(static_cast<Type_*>(_data) + _count, addend.GetData(), addend.DataSize());
        _count = newLength;
    }

    template<typename Type_>
    bool CArray<Type_>::Remove(const Type_& item)
    {
        uint32 index = 0;
        for(; index < _count; ++index) {
            if(_data[index] == item) {
                break;
            }
        }

        if(index == _count) {
            return false;
        }

        for(; index < _count; ++index) {
            _data[index] = _data[index + 1];
        }

        --_count;

        return true;
    }

    template<typename Type_>
    void CArray<Type_>::RemoveFast(uint index)
    {
        Assert_(index >= 0);
        Assert_(index < _count);

        _data[index] = _data[_count - 1];
        _count--;
    }

    template<typename Type_>
    void CArray<Type_>::ReallocateArray(uint32 newLength, uint32 newCapacity)
    {
        Type_* newList = AllocArray_(Type_, newCapacity);

        if(_data) {
            uint32 lengthToCopy = (_count < newLength) ? _count : newLength;
            if(lengthToCopy > 0) {
                Memory::Copy(newList, _data, lengthToCopy * sizeof(Type_));
            }

            Free_(_data);
        }

        _data = newList;
        _count = newLength;
        _capacity = newCapacity;
    }

    template<typename Type_>
    void CArray<Type_>::GrowArray(void)
    {
        // Idea from old BHG code; seems very sensible.
        if(_capacity < 64) {
            ReallocateArray(_count, _capacity + 16);
        }
        else if(_capacity < 256) {
            ReallocateArray(_count, _capacity + 32);
        }
        else {
            ReallocateArray(_count, _capacity + 128);
        }
    }
}