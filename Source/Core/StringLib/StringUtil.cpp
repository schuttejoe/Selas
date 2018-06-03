//==============================================================================
// StringUtil
//==============================================================================

#include <StringLib/StringUtil.h>
#include <IoLib/Environment.h>
#include <SystemLib/JsAssert.h>
#include <SystemLib/CheckedCast.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// -- for vsnprintf_s
#include <stdio.h>

namespace Selas
{
    namespace StringUtil
    {

        //==============================================================================
        char Lowercase(char c)
        {
            return CheckedCast<char>(tolower(c));
        }

        //==============================================================================
        int32 Length(cpointer text)
        {
            Assert_(text != nullptr);
            return (int32)strlen(text);
        }

        //==============================================================================
        char* FindChar(char* text, char searchChar)
        {
            return strchr(text, searchChar);
        }

        //==============================================================================
        char* FindLastChar(char* text, char searchChar)
        {
            return strrchr(text, searchChar);
        }

        //==============================================================================
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

        //==============================================================================
        cpointer FindSubString(cpointer text, char const* searchText)
        {
            return strstr(text, searchText);
        }

        //==============================================================================
        char* FindSubString(char* text, char const* searchText)
        {
            return strstr(text, searchText);
        }

        //==============================================================================
        int32 FindIndexOf(char const* text, char searchChar)
        {
            char const* location = strchr(text, searchChar);
            if(location == nullptr) {
                return -1;
            }

            return (int32)(location - text);
        }

        //==============================================================================
        int32 FindIndexOf(char const* text, char const* searchText)
        {
            return FindIndexOf(text, searchText, 0);
        }

        //==============================================================================
        int32 FindIndexOf(char const* text, char const* searchText, int32 offset)
        {
            char const* location = strstr(text + offset, searchText);
            if(location == nullptr) {
                return -1;
            }

            return (int32)(location - text);
        }

        //==============================================================================
        int32 Compare(char const* lhs, char const* rhs)
        {
            return strcmp(lhs, rhs);
        }

        //==============================================================================
        int32 CompareN(char const* lhs, char const* rhs, int32 compareLength)
        {
            return strncmp(lhs, rhs, compareLength);
        }

        //==============================================================================
        int32 CompareNIgnoreCase(char const* lhs, char const* rhs, int32 compareLength)
        {
            return _strnicmp(lhs, rhs, compareLength);
        }

        //==============================================================================
        bool Equals(char const* lhs, char const* rhs)
        {
            return (strcmp(lhs, rhs) == 0);
        }

        //==============================================================================
        bool EqualsN(char const* lhs, char const* rhs, int32 compareLength)
        {
            return (strncmp(lhs, rhs, compareLength) == 0);
        }

        //==============================================================================
        bool EqualsIgnoreCase(char const* lhs, char const* rhs)
        {
            return (_stricmp(lhs, rhs) == 0);
        }

        //==============================================================================
        bool EndsWithIgnoreCase(cpointer lhs, cpointer rhs)
        {
            uint lhsLength = Length(lhs);
            uint rhsLength = Length(rhs);

            // -- too small
            if(rhsLength > lhsLength) {
                return false;
            }

            uint startIndex = lhsLength - rhsLength;
            return _stricmp(lhs + startIndex, rhs) == 0;
        }

        //==============================================================================
        void Copy(char* destString, int32 destMaxLength, char const* sourceString)
        {
            strcpy_s(destString, destMaxLength, sourceString);
        }

        //==============================================================================
        void CopyN(char* destString, int32 destMaxLength, char const* sourceString, int32 srcStringLength)
        {
            int32 copyLength = (srcStringLength < destMaxLength) ? srcStringLength : destMaxLength - 1;

            strncpy_s(destString, destMaxLength, sourceString, copyLength);
            destString[copyLength] = '\0';
        }

        //==============================================================================
        void ReplaceAll(char* str, char charToReplace, char replacement)
        {
            char* nextChar = strchr(str, charToReplace);
            while(nextChar) {
                *nextChar = replacement;
                nextChar = strchr(nextChar, charToReplace);
            }
        }

        //==============================================================================
        int32 to_int(char const* text)
        {
            Assert_(text);
            return atoi(text);
        }

        //==============================================================================
        float ToFloat(char const* text)
        {
            Assert_(text);
            return static_cast<float>(atof(text));
        }

        //==============================================================================
        void Sprintf(char* dst, uint32 dstSize, const char* message, ...)
        {
            va_list varg;
            va_start(varg, message);

            vsnprintf_s(dst, dstSize, _TRUNCATE, message, varg);

            va_end(varg);
        }

        //==============================================================================
        void RemoveExtension(char* str)
        {
            char* last = FindLastChar(str, '.');
            if(last == nullptr) {
                return;
            }

            *last = 0;
        }

        //=================================================================================================
        bool FullPathName(cpointer src, char* dst, uint maxLength)
        {
            #if IsWindows_
            if(GetFullPathNameA(src, (DWORD)maxLength, dst, nullptr) == 0) {
                return false;
            }
            #else
                static_assert("you want realpath");
            #endif

            return true;
        }

        //=================================================================================================
        char PathSeperator()
        {
            #if IsWindows_
                return '\\';
            #else
                static_assert("you want realpath");
            #endif
        }

        //=================================================================================================
        bool SanitizePath(cpointer src, char* dst, uint maxLength)
        {
            FixedString128 projectRoot = Environment_Root();

            FixedString256 temp;
            ReturnFailure_(FullPathName(src, temp.Ascii(), temp.Capcaity()));

            uint offset = 0;

            const char* root = FindSubString(temp.Ascii(), projectRoot.Ascii());
            if(root != nullptr) {
                offset = Length(projectRoot.Ascii());
            }

            char pathSep = PathSeperator();
            ReplaceAll(temp.Ascii(), pathSep, '|');

            Copy(dst, (uint32)maxLength, temp.Ascii() + offset);

            return true;
        }

        //=================================================================================================
        cpointer LastFileOrFolderName(char* path)
        {
            cpointer searchCharacters = "/\\";
            int last = StringUtil::FindLastIndexOfAny(path, searchCharacters) + 1;
            if(last == -1) {
                return nullptr;
            }

            return &path[last];
        }

        //=================================================================================================
        void RemoveLastFileOrFolder(char* path)
        {
            cpointer searchCharacters = "/\\";
            int last = StringUtil::FindLastIndexOfAny(path, searchCharacters);
            if(last != -1) {
                path[last] = '\0';
            }
        }

        //=================================================================================================
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

        //=================================================================================================
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