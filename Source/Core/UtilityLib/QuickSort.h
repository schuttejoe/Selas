#pragma once

//==============================================================================
// Joe Schutte
//==============================================================================

#include "SystemLib/BasicTypes.h"

namespace Selas
{
    template <typename Type_>
    void QuickSort(Type_* data, uint count)
    {
        if(count < 2) {
            return;
        }

        Type_ pivot = data[count / 2];
        Type_* left = data;
        Type_* right = data + count - 1;

        while(left <= right) {
            if(*left < pivot) {
                left++;
            }
            else if(*right > pivot) {
                right--;
            }
            else {
                Type_ temp = *left;
                *left = *right;
                *right = temp;
                left++;
                right--;
            }
        }

        QuickSort(data, right - data + 1);
        QuickSort(left, data + count - left);
    }
}