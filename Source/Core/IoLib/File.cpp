//=================================================================================================================================
// Joe Schutte
//=================================================================================================================================

#include "IoLib/File.h"

#include "SystemLib/JsAssert.h"
#include "SystemLib/MemoryAllocation.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#if IsWindows_
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace Selas
{
    namespace File
    {
        //=========================================================================================================================
        static FILE* OpenFile_(const char* filepath, const char* mode)
        {
            FILE* result = nullptr;

            #if IsWindows_
                fopen_s(&result, filepath, mode);
            #elif IsOsx_
                result = fopen(filepath, mode);
            #endif

            return result;
        }

        //=========================================================================================================================
        Error ReadWholeFile(const char* filepath, void** __restrict fileData, uint64* __restrict fileSize)
        {
            FILE* file = OpenFile_(filepath, "rb");
            if(file == nullptr) {
                return Error_("Failed to open file: %s", filepath);
            }

            fseek(file, 0, SEEK_END);
            *fileSize = _ftelli64(file);
            fseek(file, 0, SEEK_SET);

            *fileData = AllocAligned_(*fileSize, 16);
            if(*fileData == nullptr) {
                return Error_("Failed to make allocation of size %u for file %s:", fileSize, filepath);
            }

            size_t bytesRead = fread(*fileData, 1, *fileSize, file);
            fclose(file);

            Assert_(bytesRead == *fileSize);
            Unused_(bytesRead);

            return Success_;
        }

        //=========================================================================================================================
        Error ReadWhileFileAsString(cpointer filepath, char** string, uint64* stringSize)
        {
            FILE* file = OpenFile_(filepath, "rb");
            if(file == nullptr) {
                return Error_("Failed to open file %s", filepath);
            }

            fseek(file, 0, SEEK_END);
            uint64 fileSize = _ftelli64(file);
            fseek(file, 0, SEEK_SET);

            *string = (char*)AllocAligned_(fileSize + 1, 16);
            if(*string == nullptr) {
                return Error_("Failed to make allocation of size %u for file %s:", fileSize + 1, filepath);
            }

            size_t bytesRead = fread(*string, 1, fileSize, file);
            fclose(file);

            Assert_(bytesRead == fileSize);
            Unused_(bytesRead);

            // -- null terminate
            (*string)[fileSize] = '\0';
            *stringSize = fileSize;

            return Success_;
        }

        //=========================================================================================================================
        Error WriteWholeFile(const char* filepath, const void* data, uint64 size)
        {
            FILE* file = OpenFile_(filepath, "wb");
            if(file == nullptr) {
                return Error_("Failed to open file: %s", filepath);
            }

            size_t bytes_written = fwrite(data, 1, size, file);
            fclose(file);

            Assert_(bytes_written == size);
            (void)bytes_written;

            return Success_;
        }

        //=========================================================================================================================
        Error Size(cpointer filepath, uint64& size)
        {
            struct stat fs = { 0 };
            if(stat(filepath, &fs) == 0) {
                size = (uint64) fs.st_size;
                return Success_;
            }

            size = (uint64)-1;
            return Error_("Failed to get file size for file %s", filepath);;
        }

        //=========================================================================================================================
        bool Exists(cpointer filepath)
        {
            #if IsWindows_
                DWORD dwAttrib = GetFileAttributesA(filepath);
                return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
            #else
                int res = access(filepath, R_OK);
                return res == 0;
            #endif
        }
    }
}
