//==============================================================================
// StringUtil
//==============================================================================

#include <StringLib/StringUtil.h>
#include <SystemLib/JsAssert.h>
#include <SystemLib/CheckedCast.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

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
        int32 Length(const char* text)
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
        int32 FindLastIndexOfAny(const char* text, const char* searchCharacters)
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
        bool EndsWithIgnoreCase(const char* lhs, const char* rhs)
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
        void RemoveExtension(char* str)
        {
            char* last = FindLastChar(str, '.');
            if(last == nullptr) {
                return;
            }

            *last = 0;
        }

        //=================================================================================================
        void GetFolderPath(const char* inpath, char* outDirectory, uint32 maxLength)
        {
            int32 found = FindLastIndexOfAny(inpath, ".");
            if(found == -1) {
                Copy(outDirectory, maxLength, inpath);
                return;
            }

            const char* searchCharacters = "/\\";
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