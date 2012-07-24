import os
import sys
import subprocess

arch = subprocess.Popen (["uname", "-p"], stdout=subprocess.PIPE).communicate ()[0][:-1]

os.environ["COMPIZ_METADATA_PATH"] = "generated/"
os.environ["COMPIZ_CONFIG_PROFILE"] = ""
os.environ["XDG_CONFIG_HOME"] = "compizconfig/libcompizconfig/config"
os.environ["LIBCOMPIZCONFIG_BACKEND_PATH"] = "compizconfig/libcompizconfig/backend/"
os.environ["XDG_DATA_DIRS"] = "generated/"

# TODO: does this really work ?
sys.path[0] = "compizconfig/compizconfig-python/build/lib.linux-" + arch + "-" + str (sys.version_info[0]) + "." + str (sys.version_info[1]) + "/"

import unittest
import compizconfig

class CompizConfigTest (unittest.TestCase):

    ccs = compizconfig

    def setUp (self):
        self.context = compizconfig.Context ()

