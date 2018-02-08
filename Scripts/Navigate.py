
import os
import sys

## ------------------------------------------------------------------------------------------------
def main(argv):
    engineDir = os.environ['ShootyEngine']

    drive, tail = os.path.splitdrive(engineDir)

    targetDir = engineDir

    if(len(argv) > 0):
        if argv[0].lower() == "source":
            targetDir = os.path.join(engineDir, "Source")
        if argv[0].lower() == "src":
            targetDir = os.path.join(engineDir, "Source")
        if argv[0].lower() == "scripts":
            targetDir = os.path.join(engineDir, "Scripts")
        if argv[0].lower() == "content":
            targetDir = os.path.join(engineDir, "Content")
        if argv[0].lower() == "pathtracer" or argv[0].lower() == "pt":
            targetDir = os.path.join(engineDir, "Source", "PathTracer")
        if argv[0].lower() == "build":
            targetDir = os.path.join(engineDir, "Source", "Tools", "Build")
        if argv[0].lower() == "projects":
            targetDir = os.path.join(engineDir, "_Projects")
        if argv[0].lower() == "middleware":
            targetDir = os.path.join(engineDir, "Middleware")

        if argv[0].lower() == "demos":
            targetDir = "D:\\Demos"
            

    print(str(drive))
    print('cd %s' % str(targetDir))

if __name__ == "__main__":
    main(sys.argv[1:])