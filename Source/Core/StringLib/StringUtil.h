#pragma once

//==============================================================================
// StringUtil
//==============================================================================

#include <SystemLib/BasicTypes.h>

namespace Selas
{
    namespace StringUtil
    {
        // JSTODO - Unify parameter ordering
        char Lowercase(char c);
        int32 Length(char const* text);

        char* FindChar(char* text, char searchChar);
        char* FindLastChar(char* text, char searchChar);

        cpointer FindSubString(cpointer text, char const* searchString);
        char* FindSubString(char* text, char const* searchString);

        int32 FindIndexOf(char const* text, char searchChar);
        int32 FindIndexOf(char const* text, char const* searchText);
        int32 FindIndexOf(char const* text, char const* searchText, int32 offset);

        int32 Compare(char const* lhs, char const* rhs);
        int32 CompareN(char const* lhs, char const* rhs, int32 compareLength);
        int32 CompareNIgnoreCase(char const* lhs, char const* rhs, int32 compareLength);

        bool Equals(char const* lhs, char const* rhs);
        bool EqualsN(char const* lhs, char const* rhs, int32 compareLength);
        bool EqualsIgnoreCase(char const* lhs, char const* rhs);

        bool EndsWithIgnoreCase(cpointer lhs, cpointer rhs);

        void Copy(char* destString, int32 destMaxLength, char const* srcString);
        void CopyN(char* destString, int32 destMaxLength, char const* srcString, int32 srcStringLength);

        void ReplaceAll(char* str, char charToReplace, char replacement);

        int32 ToInt32(char const* text);
        float ToFloat(char const* text);

        // -- Calls realpath(linux) or GetFullPathName (windows)
        bool FullPathName(cpointer src, char* dst, uint maxLength);
        char PathSeperator();

        // -- file name utilities
        cpointer LastFileOrFolderName(char* path);
        void RemoveLastFileOrFolder(char* path);

        void RemoveExtension(char* str);
        void GetFolderPath(cpointer inpath, char* outDirectory, uint32 maxLength);
        bool GetExtension(cpointer path, char* extension, uint32 maxLength);

    }
}