//==============================================================================
// Joe Schutte
//==============================================================================

#include <IoLib/File.h>

#include <SystemLib/JsAssert.h>
#include <SystemLib/MemoryAllocation.h>
#include <stdio.h>
#include <stdlib.h>

namespace Shooty {
    namespace File {

        //==============================================================================
        bool ReadWholeFile(const char* filename, void** __restrict fileData, uint32* __restrict fileSize) {
            FILE* file = nullptr;
            fopen_s(&file, filename, "rb");
            if (file == nullptr) {
                return false;
            }

            fseek(file, 0, SEEK_END);
            *fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            *fileData = AllocAligned_(*fileSize, 16);

            size_t bytesRead = fread(*fileData, 1, *fileSize, file);
            fclose(file);

            Assert_(bytesRead == *fileSize);
            Unused_(bytesRead);

            return true;
        }

        //==============================================================================
        bool WriteWholeFile(const char* filename, void* data, uint32 size) {
            FILE* file = nullptr;
            fopen_s(&file, filename, "wb");
            if (file == nullptr) {
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