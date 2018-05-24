
import os
import sys

## ------------------------------------------------------------------------------------------------
def main(argv):
    engineDir = os.environ['Selas']

    drive, tail = os.path.splitdrive(engineDir)

    targetDir = engineDir

    if(len(argv) > 0):
        if argv[0].lower() == "source":
            targetDir = os.path.join(engineDir, "Source")
        if argv[0].lower() == "core":
            targetDir = os.path.join(engineDir, "Source", "Core")
        if argv[0].lower() == "apps":
            targetDir = os.path.join(engineDir, "Source", "Applications")
        if argv[0].lower() == "src":
            targetDir = os.path.join(engineDir, "Source")
        if argv[0].lower() == "scripts":
            targetDir = os.path.join(engineDir, "Scripts")
        if argv[0].lower() == "content":
            targetDir = os.path.join(engineDir, "Content")
        if argv[0].lower() == "pathtracer" or argv[0].lower() == "pt":
            targetDir = os.path.join(engineDir, "Source", "Applications", "PathTracer")
        if argv[0].lower() == "build":
            targetDir = os.path.join(engineDir, "Source", "Applications", "Build")
        if argv[0].lower() == "projects":
            targetDir = os.path.join(engineDir, "_Projects")
        if argv[0].lower() == "middleware":
            targetDir = os.path.join(engineDir, "Middleware")
        if argv[0].lower() == "blog":
            targetDir = os.path.join(engineDir, "..", "schuttejoe.github.io")

        if argv[0].lower() == "demos":
            targetDir = "D:\\Demos"
            

    print(str(drive))
    print('cd %s' % str(targetDir))

if __name__ == "__main__":
    main(sys.argv[1:])