#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Shooty
{
    template <typename Type_>
    uint BinarySearch(Type_* __restrict data, uint count, Type_ searchKey)
    {
        uint index = (uint)-1;

        sint low = 0;
        sint high = count - 1;
        while(low <= high) {
            sint mid = (low + high) / 2;

            if(data[mid] >= searchKey) {
                index = mid;
                high = mid - 1;
            }
            else {
                low = mid + 1;
            }
        }

        if(data[index] == searchKey) {
            return index;
        }

        return (uint)-1;
    }
}