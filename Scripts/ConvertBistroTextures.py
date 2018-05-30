
import os
import glob
import json
import sys

imageMagick = "C:\\Program Files\\ImageMagick-7.0.7-Q16\\magick.exe"
textureSourceDir = "D:\\Shooty\\Selas\\Content\\Scenes\\Bistro\\Textures\\"
materialDestDir = "D:\\Shooty\\Selas\\Content\\Materials\\Bistro\\"
textureDestDir = "D:\\Shooty\\Selas\\Content\\Textures\\Bistro\\"
dstSubDir = "Bistro\\"

## ------------------------------------------------------------------------------------------------
def GatherMaterials(materials):
    os.chdir(textureSourceDir)
    for file in glob.glob("*.dds"):
        name = os.path.splitext(file)[0]
        if "_BaseColor" in name:
            name = name.replace("_0_BaseColor", "")
            materials.append(name)

## ------------------------------------------------------------------------------------------------
def CreateAlbedoAssetName(materialName):
    return dstSubDir + materialName + "_Albedo.png"

## ------------------------------------------------------------------------------------------------
def CreateAlbedoSourcePath(materialName):
    return textureSourceDir + materialName + "_0_BaseColor.dds"

## ------------------------------------------------------------------------------------------------
def CreateAlbedoDestPath(materialName):
    return textureDestDir + materialName + "_Albedo.png"

## ------------------------------------------------------------------------------------------------
def CreateSpecularAssetName(materialName):
    return dstSubDir + materialName + "_Specular.png"

## ------------------------------------------------------------------------------------------------
def CreateSpecularSourcePath(materialName):
    return textureSourceDir + materialName + "_0_Specular.dds"

## ------------------------------------------------------------------------------------------------
def CreateSpecularDestPath(materialName):
    return textureDestDir + materialName + "_Specular.png"


## ------------------------------------------------------------------------------------------------
def CreateMaterialAssets(materials):
    for materialName in materials:
        material = {}
        material["Metalness"] = 0.1
        material["AlbedoTexture"] = CreateAlbedoAssetName(materialName)

        specSrc = CreateSpecularSourcePath(materialName)
        if os.path.isfile(specSrc):
            material["SpecularTexture"] = CreateSpecularAssetName(materialName)

        if "glass" in materialName.lower():
            material["ShaderName"] = "TransparentGGX"
            material["Ior"] = 1.52
            material["Roughness"] = 0.03

        filePath = materialDestDir + materialName + ".json"
        with open(filePath, 'w') as file:
            prettyJson = json.dumps(material, indent=4)
            file.write(prettyJson)

## ------------------------------------------------------------------------------------------------
def ConvertTexture(src, dst):
    cmd = "\"" + imageMagick + "\" " + src + " " + dst
    if os.path.isfile(src):
        print(cmd)
        os.system(cmd)

## ------------------------------------------------------------------------------------------------
def ConvertTextures(materials):
    for materialName in materials:
        albedoSrc = CreateAlbedoSourcePath(materialName)
        albedoDst = CreateAlbedoDestPath(materialName)
        ConvertTexture(albedoSrc, albedoDst)

        specSrc = CreateSpecularSourcePath(materialName)
        specDst = CreateSpecularDestPath(materialName)
        ConvertTexture(specSrc, specDst)

## ------------------------------------------------------------------------------------------------
def main(argv):

    materials = []
    GatherMaterials(materials)
    CreateMaterialAssets(materials)

    # UGH - Normal maps are BC5 which is not supported by imagemagick. Guess I'll have to use DirectxTex
    # ConvertTextures(materials)

if __name__ == "__main__":
    main(sys.argv[1:])