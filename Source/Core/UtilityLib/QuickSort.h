#pragma once

//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

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

    template <typename KeyType_, typename DataType_>
    void QuickSortMatchingArrays(KeyType_* keys, DataType_* data, uint count)
    {
        if(count < 2) {
            return;
        }

        KeyType_ pivot = keys[count / 2];
        KeyType_* leftKey = keys;
        DataType_* leftData = data;
        KeyType_* rightKey = keys + count - 1;
        DataType_* rightData = data + count - 1;

        while(leftKey <= rightKey) {
            if(*leftKey < pivot) {
                leftKey++;
                leftData++;
            }
            else if(*rightKey > pivot) {
                rightKey--;
                rightData--;
            }
            else {
                KeyType_ temp = *leftKey;
                *leftKey = *rightKey;
                *rightKey = temp;

                DataType_ tempdata = *leftData;
                *leftData = *rightData;
                *rightData = tempdata;

                leftKey++;
                leftData++;
                rightKey--;
                rightData--;
            }
        }

        QuickSortMatchingArrays(keys, data, rightKey - keys + 1);
        QuickSortMatchingArrays(leftKey, leftData, keys + count - leftKey);
    }
}