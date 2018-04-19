//==============================================================================
// Joe Schutte
//==============================================================================

#include <BuildCommon/BuildMaterial.h>
#include <BuildCommon/SceneBuildPipeline.h>
#include <StringLib/StringUtil.h>
#include <MathLib/FloatFuncs.h>
#include <SystemLib/MemoryAllocation.h>


namespace Shooty
{
    bool ImportMaterial(const char* materialName, ImportedMaterialData* material)
    {
        // -- This is all hacks until I figure out where I want to get real materials. Export from substance designer?
        material->emissiveTexture.Clear();

        if(StringUtil::EqualsIgnoreCase(materialName, "HDR")) {
            material->emissiveTexture.Copy("D:\\Shooty\\ShootyEngine\\_Assets\\Textures\\red_wall_4k");
        }
        else {
            material->albedoTexture.Copy("D:\\Shooty\\ShootyEngine\\_Assets\\Textures\\CommonWhite");
        }
        return true;
    }
}