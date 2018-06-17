
## ------------------------------------------------------------------------------------------------
# 

import os
import glob
import json
import sys
import math

PlatformIndependencyPathSep = '~'

class BlinnPhongMaterial:
    def __init__(self, name):
        self.name = name
        self.diffuse = [1, 1, 1]
        self.ambient = [0, 0, 0]
        self.specular = [0, 0, 0]
        self.emissive = [0, 0, 0]
        self.diffusemap = ''
        self.bumpmap = ''
        self.specularmap = ''
        self.exponent = 1
        self.ior = 1.0
        self.illum = 0
        self.transmissive = 1

## ------------------------------------------------------------------------------------------------
def ParseFloat3Arg(args):
    x = float(args[0])
    y = float(args[1])
    z = float(args[2])
    return [x, y, z]

## ------------------------------------------------------------------------------------------------
def NewMaterial(name):
    return BlinnPhongMaterial(name)

## ------------------------------------------------------------------------------------------------
def Kd(material, args):
    material.diffuse = ParseFloat3Arg(args[1:])

## ------------------------------------------------------------------------------------------------
def Ka(material, args):
    material.ambient = ParseFloat3Arg(args[1:])

## ------------------------------------------------------------------------------------------------
def Ks(material, args):
    material.specular = ParseFloat3Arg(args[1:])

## ------------------------------------------------------------------------------------------------
def Ke(material, args):
    material.emissive = ParseFloat3Arg(args[1:])

## ------------------------------------------------------------------------------------------------
def map_Kd(material, args):
    material.diffusemap = args[1]

## ------------------------------------------------------------------------------------------------
def map_Bump(material, args):
    material.bumpmap = args[1]

## ------------------------------------------------------------------------------------------------
def map_bump(material, args):
    material.bumpmap = args[1]

## ------------------------------------------------------------------------------------------------
def map_Ks(material, args):
    material.specularmap = args[1]

## ------------------------------------------------------------------------------------------------
def Ns(material, args):
    material.exponent = float(args[1])

## ------------------------------------------------------------------------------------------------
def Ni(material, args):
    material.ior = float(args[1])

## ------------------------------------------------------------------------------------------------
def d(material, args):
    material.opacity = float(args[1])

## ------------------------------------------------------------------------------------------------
def illum(material, args):
    material.illum = int(args[1])

## ------------------------------------------------------------------------------------------------
def Tf(material, args):
    material.transmissive = ParseFloat3Arg(args[1:])

## ------------------------------------------------------------------------------------------------
def ParseMtlFile(filepath):
    materials = []

    print("Opening file %s" % filepath)
    with open(filepath, "r") as file:
        print("Beginning parsing...")
    
        currentMaterial = None

        for line in file:
            if line.startswith("#"):
                continue

            splitLine = line.split()
            if len(splitLine) == 0:
                continue

            if splitLine[0] == 'newmtl':
                currentMaterial = NewMaterial(splitLine[1])
                materials.append(currentMaterial)
            else:
                func = globals()[splitLine[0]]
                func(currentMaterial, splitLine)

    return materials

## ------------------------------------------------------------------------------------------------
def CreateMaterialAssets(outputFolder, texturePrefix, materials):
    print("Writing materials to: %s" % outputFolder)
    print("Writing materials with texture prefix: %s" % texturePrefix)

    for blinnPhongMat in materials:
        
        materialName = blinnPhongMat.name

        material = {}
        
        if blinnPhongMat.illum in [4, 6, 7, 9]:
            material["ShaderName"] = "TransparentGGX"
        else:
            material["ShaderName"] = "Disney" # Currently this is lies. Should really get rid of this...

        material["Metalness"] = 0.1

        material["Albedo"] = blinnPhongMat.diffuse

        if blinnPhongMat.diffusemap != "":
            material["AlbedoTexture"] = os.path.join(texturePrefix, os.path.normpath(blinnPhongMat.diffusemap)).replace(os.sep, PlatformIndependencyPathSep)

        if blinnPhongMat.bumpmap != "" and blinnPhongMat.bumpmap.startswith("N_"): # hack because SanMiguel scenes have some bump maps which I don't currently support.
            material["NormalTexture"] = os.path.join(texturePrefix, os.path.normpath(blinnPhongMat.bumpmap)).replace(os.sep, PlatformIndependencyPathSep)

        # Convert Blinn-phong shineyness to a roughness parameter
        material["Roughness"] = math.sqrt(2.0 / (blinnPhongMat.exponent + 2))
        
        filePath = os.path.join(outputFolder, materialName + '.json')
        with open(filePath, 'w') as file:
            prettyJson = json.dumps(material, indent=4)
            file.write(prettyJson)

## ------------------------------------------------------------------------------------------------
def main(argv):
    if len(argv) < 1:
        print("Missing command line arguments. Expected use is \'MtlToMaterials.py <MtlSourceFile>\'")
        return

    normalizedRoot = os.path.normpath(argv[0])

    contentRoot = normalizedRoot
    while contentRoot.rsplit(os.sep, 1)[-1] != 'Content':
        contentRoot = contentRoot.rsplit(os.sep, 1)[0]

    texturePrefix = os.path.dirname(normalizedRoot)[len(contentRoot) + 1:]
    outputFolder = os.path.join(os.path.dirname(argv[0]), "Materials")

    materials = ParseMtlFile(argv[0])

    CreateMaterialAssets(outputFolder, texturePrefix, materials)

if __name__ == "__main__":
    main(sys.argv[1:])