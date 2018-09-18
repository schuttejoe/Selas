//=================================================================================================================================
// StringUtil
//=================================================================================================================================

#include "StringLib/StringUtil.h"
#include "StringLib/FixedString.h"
#include "SystemLib/JsAssert.h"
#include "SystemLib/CheckedCast.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#if IsWindows_
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#define SelasPathMax_ 256

// -- for vsnprintf_s
#include <stdio.h>

namespace Selas
{
    namespace StringUtil
    {

        //=========================================================================================================================
        char Lowercase(char c)
        {
            return CheckedCast<char>(tolower(c));
        }

        //=========================================================================================================================
        int32 Length(cpointer text)
        {
            Assert_(text != nullptr);
            return (int32)strlen(text);
        }

        //=========================================================================================================================
        char* FindChar(char* text, char searchChar)
        {
            return strchr(text, searchChar);
        }

        //=========================================================================================================================
        char* FindLastChar(char* text, char searchChar)
        {
            return strrchr(text, searchChar);
        }

        //=========================================================================================================================
        int32 FindLastIndexOfAny(cpointer text, cpointer searchCharacters)
        {
            if(text == nullptr || searchCharacters == nullptr) {
                return -1;
            }

            int32 len = Length(text);

            for(int32 textIndex = len - 1; textIndex >= 0; --textIndex) {
                for(int32 searchIndex = 0; searchCharacters[searchIndex]; ++searchIndex) {
                    if(text[textIndex] == searchCharacters[searchIndex]) {
                        return textIndex;
                    }
                }
            }

            return -1;
        }

        //=========================================================================================================================
        cpointer FindSubString(cpointer text, char const* searchText)
        {
            return strstr(text, searchText);
        }

        //=========================================================================================================================
        char* FindSubString(char* text, char const* searchText)
        {
            return strstr(text, searchText);
        }

        //=========================================================================================================================
        int32 FindIndexOf(char const* text, char searchChar)
        {
            char const* location = strchr(text, searchChar);
            if(location == nullptr) {
                return -1;
            }

            return (int32)(location - text);
        }

        //=========================================================================================================================
        int32 FindIndexOf(char const* text, char const* searchText)
        {
            return FindIndexOf(text, searchText, 0);
        }

        //=========================================================================================================================
        int32 FindIndexOf(char const* text, char const* searchText, int32 offset)
        {
            char const* location = strstr(text + offset, searchText);
            if(location == nullptr) {
                return -1;
            }

            return (int32)(location - text);
        }

        //=========================================================================================================================
        int32 Compare(char const* lhs, char const* rhs)
        {
            return strcmp(lhs, rhs);
        }

        //=========================================================================================================================
        int32 CompareN(char const* lhs, char const* rhs, int32 compareLength)
        {
            return strncmp(lhs, rhs, compareLength);
        }

        //=========================================================================================================================
        int32 CompareNIgnoreCase(char const* lhs, char const* rhs, int32 compareLength)
        {
            #if IsWindows_
            return _strnicmp(lhs, rhs, compareLength);
            #elif IsOsx_
                return strncmp(lhs, rhs, compareLength);
            #endif
        }

        //=========================================================================================================================
        bool Equals(char const* lhs, char const* rhs)
        {
            return (strcmp(lhs, rhs) == 0);
        }

        //=========================================================================================================================
        bool EqualsN(char const* lhs, char const* rhs, int32 compareLength)
        {
            return (strncmp(lhs, rhs, compareLength) == 0);
        }

        //=========================================================================================================================
        bool EqualsIgnoreCase(char const* lhs, char const* rhs)
        {
            #if IsWindows_
            return (_stricmp(lhs, rhs) == 0);
            #elif IsOsx_
                return (strcmp(lhs, rhs) == 0);
            #endif
        }

        //=========================================================================================================================
        bool EndsWithIgnoreCase(cpointer lhs, cpointer rhs)
        {
            uint lhsLength = Length(lhs);
            uint rhsLength = Length(rhs);

            // -- too small
            if(rhsLength > lhsLength) {
                return false;
            }

            uint startIndex = lhsLength - rhsLength;

            #if IsWindows_
                return _stricmp(lhs + startIndex, rhs) == 0;
            #elif IsOsx_
                return strcmp(lhs + startIndex, rhs) == 0;
            #endif
        }

        //=========================================================================================================================
        void Copy(char* destString, int32 destMaxLength, char const* sourceString)
        {
            #if IsWindows_
            strcpy_s(destString, destMaxLength, sourceString);
            #elif IsOsx_
                strncpy(destString, sourceString, destMaxLength);
            #endif
        }

        //=========================================================================================================================
        void CopyN(char* destString, int32 destMaxLength, char const* sourceString, int32 srcStringLength)
        {
            int32 copyLength = (srcStringLength < destMaxLength) ? srcStringLength : destMaxLength - 1;

            #if IsWindows_
                strncpy_s(destString, destMaxLength, sourceString, copyLength);
            #elif IsOsx_
                strncpy(destString, sourceString, copyLength);
            #endif

            destString[copyLength] = '\0';
        }

        //=========================================================================================================================
        void ReplaceAll(char* str, char charToReplace, char replacement)
        {
            char* nextChar = strchr(str, charToReplace);
            while(nextChar) {
                *nextChar = replacement;
                nextChar = strchr(nextChar, charToReplace);
            }
        }

        //=========================================================================================================================
        int32 ToInt32(char const* text)
        {
            Assert_(text);
            return atoi(text);
        }

        //=========================================================================================================================
        float ToFloat(char const* text)
        {
            Assert_(text);
            return static_cast<float>(atof(text));
        }

        //=========================================================================================================================
        void Sprintf(char* dst, uint32 dstSize, const char* message, ...)
        {
            va_list varg;
            va_start(varg, message);

            #if IsWindows_
                vsnprintf_s(dst, dstSize, _TRUNCATE, message, varg);
            #else
                vsnprintf(dst, dstSize, message, varg);
            #endif

            va_end(varg);
        }

        //=========================================================================================================================
        void Sprintf(char* dst, uint dstSize, const char* message, ...)
        {
            va_list varg;
            va_start(varg, message);

            #if IsWindows_
                vsnprintf_s(dst, dstSize, _TRUNCATE, message, varg);
            #else
                vsnprintf(dst, dstSize, message, varg);
            #endif

            va_end(varg);
        }

        //=========================================================================================================================
        void RemoveExtension(char* str)
        {
            char* last = FindLastChar(str, '.');
            if(last == nullptr) {
                return;
            }

            *last = 0;
        }

        //====================================================================================================================================
        bool FullPathName(cpointer src, char* dst, uint maxLength)
        {
            // -- Don't be stringy.
            Assert_(maxLength >= SelasPathMax_);

            #if IsWindows_
            if(GetFullPathNameA(src, (DWORD)maxLength, dst, nullptr) == 0) {
                return false;
            }
            #elif IsOsx_
                if(realpath(src, dst) == 0) {
                    return false;
                }
            #endif

            return true;
        }

        //=========================================================================================================================
        char PathSeperator()
        {
            #if IsWindows_
                return '\\';
            #else
                return '/';
            #endif
        }

        //=========================================================================================================================
        bool SanitizePath(cpointer root, char pathSep, cpointer src, char* dst, uint maxLength)
        {
            FixedString256 temp;
            ReturnFailure_(FullPathName(src, temp.Ascii(), temp.Capacity()));

            uint offset = 0;

            const char* rootAddr = FindSubString(temp.Ascii(), root);
            if(rootAddr != nullptr) {
                offset = Length(root);
            }

            ReplaceAll(temp.Ascii(), '\\', pathSep);
            ReplaceAll(temp.Ascii(), '/', pathSep);

            Copy(dst, (uint32)maxLength, temp.Ascii() + offset);

            return true;
        }

        //=========================================================================================================================
        cpointer LastFileOrFolderName(char* path)
        {
            cpointer searchCharacters = "/\\";
            int last = StringUtil::FindLastIndexOfAny(path, searchCharacters) + 1;
            if(last == -1) {
                return nullptr;
            }

            return &path[last];
        }

        //=========================================================================================================================
        void RemoveLastFileOrFolder(char* path)
        {
            cpointer searchCharacters = "/\\";
            int last = StringUtil::FindLastIndexOfAny(path, searchCharacters);
            if(last != -1) {
                path[last] = '\0';
            }
        }

        //=========================================================================================================================
        void GetFolderPath(cpointer inpath, char* outDirectory, uint32 maxLength)
        {
            int32 found = FindLastIndexOfAny(inpath, ".");
            if(found == -1) {
                Copy(outDirectory, maxLength, inpath);
                return;
            }

            cpointer searchCharacters = "/\\";
            int last = StringUtil::FindLastIndexOfAny(inpath, searchCharacters) + 1;
            StringUtil::CopyN(outDirectory, maxLength, inpath, last);
        }

        //=========================================================================================================================
        bool GetExtension(cpointer path, char* extension, uint32 maxLength)
        {
            int32 index = FindLastIndexOfAny(path, ".");
            if(index == -1) {
                return false;
            }

            cpointer addr = path + index + 1;
            int32 length = Length(addr);

            StringUtil::CopyN(extension, maxLength, addr, length);
            return true;
        }

    }
}