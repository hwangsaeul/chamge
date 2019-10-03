#!/usr/bin/env python3

import os
import subprocess
import sys

name = 'org.hwangsaeul.Chamge1.' + sys.argv[1]

subprocess.call([
  'gdbus-codegen',
  '--interface-prefix=' + name + '.',
  '--generate-c-code=' + os.path.join(sys.argv[3], sys.argv[2]),
  '--c-namespace=ChamgeDBus',
  '--annotate', name, 'org.gtk.GDBus.C.Name', sys.argv[1],
  sys.argv[4]
])
