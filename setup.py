# -*- coding: utf-8 -*-
from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext
import os
import subprocess

VERSION_FILE = os.path.join (os.path.dirname (__file__), "VERSION")

pkgconfig_libs = subprocess.Popen (["pkg-config", "--libs", "libcompizconfig"], stdout=subprocess.PIPE, stderr=open(os.devnull, 'w')).communicate ()[0]

if len (pkgconfig_libs) is 0:
  print "CompizConfig Python [ERROR]: No libcompizconfig.pc found in the pkg-config search path"
  print "Ensure that libcompizonfig is installed or libcompizconfig.pc is in your $PKG_CONFIG_PATH"

libs = pkgconfig_libs[2:].split (" ")[0]

setup (
  name = "compizconfig-python",
  version = open (VERSION_FILE).read ().split ("=")[1],
  ext_modules=[ 
    Extension ("compizconfig", ["src/compizconfig.pyx"],
	       library_dirs = [libs],
	       runtime_library_dirs = [libs],
               libraries = ["compizconfig"])
    ],
  cmdclass = {'build_ext': build_ext}
)

