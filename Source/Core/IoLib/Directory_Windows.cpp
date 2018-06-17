//==============================================================================
// Joe Schutte
//==============================================================================

#if IsWindows_

#include "IoLib/Directory.h"
#include "StringLib/FixedString.h"
#include "SystemLib/MinMax.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Selas
{
    namespace Directory
    {
        //==============================================================================
        void EnsureDirectoryExists(const char* path)
        {
            FixedString512 folderName;
            StringUtil::GetFolderPath(path, folderName.Ascii(), (uint32)folderName.Capacity());

            FixedString512 tempName;
            StringUtil::Copy(tempName.Ascii(), (uint32)tempName.Capacity(), folderName.Ascii());

            char* current = tempName.Ascii();
            while(current != nullptr) {
                char* fSlash = StringUtil::FindChar(current + 1, '/');
                char* bSlash = StringUtil::FindChar(current + 1, '\\');
                if(fSlash == nullptr && bSlash == nullptr) {
                    break;
                }
                if(fSlash == nullptr)
                    current = bSlash;
                else if(bSlash == nullptr)
                    current = fSlash;
                else
                    current = Min<char*>(fSlash, bSlash);

                *current = 0;
                ::CreateDirectoryA(tempName.Ascii(), nullptr);
                *current = '/';
            }

            ::CreateDirectoryA(folderName.Ascii(), nullptr);
        }
    }
}
#endif