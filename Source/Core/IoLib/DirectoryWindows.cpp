//==============================================================================
// Joe Schutte
//==============================================================================

#if IsWindows_

#include <IoLib/Directory.h>
#include <StringLib/FixedString.h>
#include <SystemLib/MinMax.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Shooty
{
    namespace Directory
    {
        //==============================================================================
        void CreateDirectoryTree(const char* path)
        {
            FixedString512 tempName;
            tempName.Copy(path);

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

            ::CreateDirectoryA(path, nullptr);
        }
    }
}
#endif