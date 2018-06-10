
import os
import sys
import time

import pyperclip

## ------------------------------------------------------------------------------------------------
def main(argv):
    ts = int(time.time())
    print("Copying %sul to clipboard" % str(ts))

    pyperclip.copy(str(ts) + "ul")

if __name__ == "__main__":
    main(sys.argv[1:])