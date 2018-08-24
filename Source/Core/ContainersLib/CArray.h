#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "IoLib/Serializer.h"
#include "SystemLib/MemoryAllocation.h"
#include "SystemLib/Memory.h"
#include "SystemLib/JsAssert.h"

namespace Selas
{
    enum ArrayFlags
    {
        eReadOnly = 0x01,
        eAttached = 0x02
    };

    template <typename Type_>
    class CArray
    {
    public:
        CArray(void);
        ~CArray(void);

        void Shutdown(void);
        void Clear(void);
        void Reserve(uint64 capacity);
        void Resize(uint64 length);

        const Type_* DataPointer(void) const { return _data; }
              Type_* DataPointer(void) { return _data; }

        inline Type_&       operator[] (uint index) { return _data[index]; }
        inline const Type_& operator[] (uint index) const { return _data[index]; }

        inline uint64 Count(void) const { return _count; }
        inline uint64 Capacity(void) const { return _capacity; }
        inline uint64 DataSize(void) const { return _count * sizeof(Type_); }

        Type_& Add(void);
        uint64 Add(const Type_& element);

        template <typename OtherType_>
        void   Append(const OtherType_& addend);

        bool Remove(const Type_& item);
        void RemoveFast(uint index);

        void Serialize(CSerializer* serializer);

    private:
        void ReallocateArray(uint64 newLength, uint64 newCapacity);
        void GrowArray(void);

    private:
        uint32 _flags;
        uint32 _padding;
        uint64 _count;
        uint64 _capacity;
        Type_* _data;
    };

    template<typename Type_>
    CArray<Type_>::CArray(void)
        : _flags(0)
        , _padding(0)
        , _count(0)
        , _capacity(0)
        , _data(nullptr)   
    {
    }

    template<typename Type_>
    CArray<Type_>::~CArray(void)
    {
        Shutdown();
    }

    template<typename Type_>
    void CArray<Type_>::Shutdown(void)
    {
        if(((_flags & eAttached) == 0) && _data) {
            Free_(_data);
        }

        _data = nullptr;
        _count = 0;
        _capacity = 0;
        _flags = 0;
    }

    template<typename Type_>
    void CArray<Type_>::Clear(void)
    {
        _count = 0;
    }

    template<typename Type_>
    void CArray<Type_>::Reserve(uint64 capacity)
    {
        Assert_((_flags & eReadOnly) == 0);
        if(capacity > _capacity) {
            ReallocateArray(_count, capacity);
        }
    }

    template<typename Type_>
    void CArray<Type_>::Resize(uint64 length)
    {
        Assert_((_flags & eReadOnly) == 0);

        if(length > _capacity) {
            ReallocateArray(length, length);
        }
        else {
            _count = length;
        }
    }

    template<typename Type_>
    Type_& CArray<Type_>::Add(void)
    {
        Assert_((_flags & eReadOnly) == 0);

        if(_count == _capacity) {
            GrowArray();
        }

        Assert_(_count < _capacity);
        uint64 index = _count++;
        return _data[index];
    }

    template<typename Type_>
    uint64 CArray<Type_>::Add(const Type_& element)
    {
        Assert_((_flags & eReadOnly) == 0);

        if(_count == _capacity) {
            GrowArray();
        }

        Assert_(_count < _capacity);
        _data[_count] = element;
        return _count++;
    }

    template<typename Type_>
    template<typename OtherType_>
    void CArray<Type_>::Append(const OtherType_& addend)
    {
        Assert_((_flags & eReadOnly) == 0);

        uint64 newLength = _count + addend.Count();

        if(_capacity < newLength)
            ReallocateArray(_count, newLength);

        Memory::Copy(static_cast<Type_*>(_data) + _count, addend.DataPointer(), addend.DataSize());
        _count = newLength;
    }

    template<typename Type_>
    bool CArray<Type_>::Remove(const Type_& item)
    {
        Assert_((_flags & eReadOnly) == 0);

        uint64 index = 0;
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
        Assert_((_flags & eReadOnly) == 0);
        Assert_(index >= 0);
        Assert_(index < _count);

        _data[index] = _data[_count - 1];
        _count--;
    }

    template<typename Type_>
    void CArray<Type_>::ReallocateArray(uint64 newLength, uint64 newCapacity)
    {
        Assert_((_flags & eReadOnly) == 0);
        Type_* newList = AllocArray_(Type_, newCapacity);

        if(_data) {
            uint64 lengthToCopy = (_count < newLength) ? _count : newLength;
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
        Assert_((_flags & eReadOnly) == 0);

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

    template<typename Type_>
    void CArray<Type_>::Serialize(CSerializer* serializer)
    {
        serializer->Serialize(&_flags, sizeof(_flags));
        serializer->Serialize(&_padding, sizeof(_padding));
        serializer->Serialize(&_count, sizeof(_count));
        serializer->Serialize(&_capacity, sizeof(_capacity));
        serializer->SerializePtr((void*&)_data, DataSize(), 0);

        if(serializer->Flags() & eSerializerAttaching) {
            _flags = eReadOnly | eAttached;
        }
    }

    template<typename Type_>
    void Serialize(CSerializer* serializer, CArray<Type_>& data)
    {
        data.Serialize(serializer);
    }
}