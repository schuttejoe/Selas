//==============================================================================
// Joe Schutte
//==============================================================================

#include <IoLib/File.h>

#include <SystemLib/JsAssert.h>
#include <SystemLib/MemoryAllocation.h>
#include <stdio.h>
#include <stdlib.h>

namespace Selas
{
    namespace File
    {
        //==============================================================================
        bool ReadWholeFile(const char* filepath, void** __restrict fileData, uint32* __restrict fileSize)
        {
            FILE* file = nullptr;
            fopen_s(&file, filepath, "rb");
            if(file == nullptr) {
                return false;
            }

            fseek(file, 0, SEEK_END);
            *fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            *fileData = AllocAligned_(*fileSize, 16);
            if(*fileData == nullptr) {
                return false;
            }

            size_t bytesRead = fread(*fileData, 1, *fileSize, file);
            fclose(file);

            Assert_(bytesRead == *fileSize);
            Unused_(bytesRead);

            return true;
        }

        //==============================================================================
        bool ReadWhileFileAsString(cpointer filepath, char** string, uint32* stringSize)
        {
            FILE* file = nullptr;
            fopen_s(&file, filepath, "rb");
            if(file == nullptr) {
                return false;
            }

            fseek(file, 0, SEEK_END);
            int32 fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            *string = (char*)AllocAligned_(fileSize + 1, 16);
            if(*string == nullptr) {
                return false;
            }

            size_t bytesRead = fread(*string, 1, fileSize, file);
            fclose(file);

            Assert_(bytesRead == fileSize);
            Unused_(bytesRead);

            // -- null terminate
            (*string)[fileSize] = '\0';

            return true;
        }

        //==============================================================================
        bool WriteWholeFile(const char* fileepath, void* data, uint32 size)
        {
            FILE* file = nullptr;
            fopen_s(&file, fileepath, "wb");
            if(file == nullptr) {
                return false;
            }

            size_t bytes_written = fwrite(data, 1, size, file);
            fclose(file);

            Assert_(bytes_written == size);
            (void)bytes_written;

            return true;
        }
    }
}