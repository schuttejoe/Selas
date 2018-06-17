//==============================================================================
// Joe Schutte
//==============================================================================

#if IsOsx_

#include "IoLib/Directory.h"
#include "StringLib/FixedString.h"
#include "SystemLib/MinMax.h"

#include <sys/stat.h>

namespace Selas
{
    namespace Directory
    {
        //==============================================================================
        static bool CreateDirectory(const char* path)
        {
            int result = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if(result == -1) {
                return false;
            }

            return true;
        }
        
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
                CreateDirectory(tempName.Ascii());
                *current = '/';
            }

            CreateDirectory(folderName.Ascii());
        }
    }
}
#endif