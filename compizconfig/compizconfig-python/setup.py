# -*- coding: utf-8 -*-
from distutils.core import setup, Command
from distutils.command.build import build as _build
from distutils.command.install import install as _install
from distutils.command.install_data import install_data as _install_data
from distutils.command.sdist import sdist as _sdist
from distutils.extension import Extension
import os
import subprocess

import unittest
import os

pkg_config_environ = os.environ
pkg_config_environ["PKG_CONFIG_PATH"] = os.getcwd () + "/../libcompizconfig:" + os.environ.get ("PKG_CONFIG_PATH", '')

# If src/compizconfig.pyx exists, build using Cython
if os.path.exists (os.getcwd () + "/src/compizconfig.pyx"):
    from Cython.Distutils import build_ext
    print "using pyx"
    ext_module_src = os.getcwd () + "/src/compizconfig.pyx"
else: # Otherwise build directly from C source
    from distutils.command.build_ext import build_ext
    ext_module_src = os.getcwd () + "/compizconfig.c"

version_file = open ("VERSION", "r")
version = version_file.read ().strip ()
if "=" in version:
    version = version.split ("=")[1]

def pkgconfig(*packages, **kw):
    flag_map = {'-I': 'include_dirs', '-L': 'library_dirs', '-l': 'libraries', '-R': 'runtime_library_dirs'}
    cmd = ['pkg-config', '--libs', '--cflags']

    tokens = subprocess.Popen (cmd + list(packages), stdout=subprocess.PIPE, env=pkg_config_environ).communicate()[0].split ()

    for t in tokens:
        if '-L' in t[:2]:
            kw.setdefault (flag_map.get ("-L"), []).append (t[2:])
            if not os.getenv ("COMPIZ_DISABLE_RPATH") is "1":
                kw.setdefault (flag_map.get ("-R"), []).append (t[2:])
        elif '-I' in t[:2]:
            kw.setdefault (flag_map.get ("-I"), []).append (t[2:])
        elif '-l' in t[:2]:
            kw.setdefault (flag_map.get ("-l"), []).append (t[2:])
    
    return kw

VERSION_FILE = os.path.join (os.path.dirname (__file__), "VERSION")

pkgconfig_libs = subprocess.Popen (["pkg-config", "--libs", "libcompizconfig_internal"], stdout=subprocess.PIPE, env=pkg_config_environ, stderr=open(os.devnull, 'w')).communicate ()[0]

if len (pkgconfig_libs) is 0:
  print ("CompizConfig Python [ERROR]: No libcompizconfig_internal.pc found in the pkg-config search path")
  print ("Ensure that libcompizonfig is installed or libcompizconfig.pc is in your $PKG_CONFIG_PATH")
  exit (1);
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
            print ("Uninstalling %s" % file)
            try:
                os.unlink (file)
            except:
                self.warn ("Could not remove file %s" % file)

class sdist (_sdist):

    def run (self):
        # Build C file
        if os.path.exists ("src/compizconfig.pyx"):
            from Cython.Compiler.Main import compile as cython_compile
            cython_compile ("src/compizconfig.pyx", output_file=os.getcwd () + "/compizconfig.c")
        # Run regular sdist
        _sdist.run (self)

    def add_defaults (self):
        _sdist.add_defaults (self)
        # Remove pyx source and add c source
        if os.path.exists ("src/compizconfig.pyx"):
            self.filelist.exclude_pattern ("src/compizconfig.pyx")
            self.filelist.append ("compizconfig.c")

class test (Command):
    description = "run tests"
    user_options = []

    def initialize_options (self):
	self.cwd = None

    def finalize_options (self):
	self.cwd = os.getcwd ()

    def run (self):
	assert os.getcwd () == self.cwd, 'Must be in package root: %s' % self.cwd
	loader = unittest.TestLoader ()

	tests = loader.discover (os.getcwd (), pattern='test*')
	result = unittest.TestResult ()

	unittest.TextTestRunner (verbosity=2).run (tests)


setup (
  name = "compizconfig-python",
  version = version,
  description      = "CompizConfig Python",
  url              = "http://www.compiz.org/",
  license          = "GPL",
  maintainer	   = "Guillaume Seguin",
  maintainer_email = "guillaume@segu.in",
  cmdclass         = {"uninstall" : uninstall,
                      "install" : install,
                      "install_data" : install_data,
                      "build_ext" : build_ext,
                      "sdist" : sdist,
		      "test"  : test},
  ext_modules=[ 
    Extension ("compizconfig", [ext_module_src],
	       **pkgconfig("libcompizconfig_internal"))
    ]
)

