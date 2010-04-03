from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext
import os

VERSION_FILE = os.path.join (os.path.dirname (__file__), "VERSION")

setup (
  name = "compizconfig-python",
  version = open (VERSION_FILE).read ().split ("=")[1],
  ext_modules=[ 
    Extension ("compizconfig", ["src/compizconfig.pyx"],
               libraries = ["compizconfig"])
    ],
  cmdclass = {'build_ext': build_ext}
)

