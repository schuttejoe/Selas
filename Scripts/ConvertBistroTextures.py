
import os
import glob
import json
import sys

imageMagick = "C:\\Program Files\\ImageMagick-7.0.7-Q16\\magick.exe"
textureSourceDir = "D:\\Shooty\\ShootyEngine\\Content\\Scenes\\Bistro\\Textures\\"
materialDestDir = "D:\\Shooty\\ShootyEngine\\Content\\Materials\\Bistro\\"
textureDestDir = "D:\\Shooty\\ShootyEngine\\Content\\Textures\\Bistro\\"
dstSubDir = "Bistro\\"

## ------------------------------------------------------------------------------------------------
def GatherMaterials(materials):
    os.chdir(textureSourceDir)
    for file in glob.glob("*.dds"):
        name = os.path.splitext(file)[0]
        if "_BaseColor" in name:
            name = name.replace("_BaseColor", "")
            materials.append(name)

## ------------------------------------------------------------------------------------------------
def CreateMaterialAssets(materials):
    for materialName in materials:
        material = {}
        material["albedo"] = dstSubDir + materialName + "_Albedo.png"
        material["normal"] = dstSubDir + materialName + "_Normal.png"
        # material["specular"] = dstSubDir + materialName + "_Specular.png"
        material["MetalnessScale"] = 0.1

        filePath = materialDestDir + materialName + ".json"
        with open(filePath, 'w') as file:
            prettyJson = json.dumps(material, indent=4)
            file.write(prettyJson)

## ------------------------------------------------------------------------------------------------
def ConvertTextures(materials):
    for materialName in materials:

        albedoSrc = textureSourceDir + materialName + "_BaseColor.dds"
        albedoDst = textureDestDir + materialName + "_Albedo.png"

        normalSrc = textureSourceDir + materialName + "_Normal.dds"
        normalDst = textureDestDir + materialName + "_Normal.png"

        # specularDst = textureSourceDir + materialName + "_Specular.png"

        albedoCmd = "\"" + imageMagick + "\" " + albedoSrc + " " + albedoDst
        normalCmd = "\"" + imageMagick + "\" " + normalSrc + " " + normalDst
        print(albedoCmd)
        # os.system(albedoCmd)
        print(normalCmd)
        os.system(normalCmd)

## ------------------------------------------------------------------------------------------------
def main(argv):

    materials = []
    GatherMaterials(materials)
    # CreateMaterialAssets(materials)

    # UGH - Normal maps are BC5 which is not supported by imagemagick. Guess I'll have to use DirectxTex
    # ConvertTextures(materials)

if __name__ == "__main__":
    main(sys.argv[1:])