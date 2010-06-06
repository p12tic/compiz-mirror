# -*- coding: utf-8 -*-
from distutils.core import setup
from distutils.command.install import install as _install
from distutils.command.install_data import install_data as _install_data
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

INSTALLED_FILES = "installed_files"

class install (_install):

    def run (self):
        _install.run (self)
        outputs = self.get_outputs ()
        length = 0
        if self.root:
            length += len (self.root)
        if self.prefix:
            length += len (self.prefix)
        if length:
            for counter in xrange (len (outputs)):
                outputs[counter] = outputs[counter][length:]
        data = "\n".join (outputs)
        try:
            file = open (INSTALLED_FILES, "w")
        except:
            self.warn ("Could not write installed files list %s" % \
                       INSTALLED_FILES)
            return 
        file.write (data)
        file.close ()

class install_data (_install_data):

    def run (self):
        def chmod_data_file (file):
            try:
                os.chmod (file, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
            except:
                self.warn ("Could not chmod data file %s" % file)
        _install_data.run (self)
        map (chmod_data_file, self.get_outputs ())

class uninstall (_install):

    def run (self):
        try:
            file = open (INSTALLED_FILES, "r")
        except:
            self.warn ("Could not read installed files list %s" % \
                       INSTALLED_FILES)
            return 
        files = file.readlines ()
        file.close ()
        prepend = ""
        if self.root:
            prepend += self.root
        if self.prefix:
            prepend += self.prefix
        if len (prepend):
            for counter in xrange (len (files)):
                files[counter] = prepend + files[counter].rstrip ()
        for file in files:
            print "Uninstalling %s" % file
            try:
                os.unlink (file)
            except:
                self.warn ("Could not remove file %s" % file)

setup (
  name = "compizconfig-python",
  version = open (VERSION_FILE).read ().split ("=")[1],
  ext_modules=[ 
    Extension ("compizconfig", ["src/compizconfig.pyx"],
	       library_dirs = [libs],
               libraries = ["compizconfig"])
    ],
    cmdclass         = {"uninstall" : uninstall,
                        "install" : install,
                        "install_data" : install_data,
                        "build_ext" : build_ext}
)

