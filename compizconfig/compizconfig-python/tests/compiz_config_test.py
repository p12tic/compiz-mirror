import unittest
import compizconfig
import os

class CompizConfigTest (unittest.TestCase):

    ccs = compizconfig

    def setUp (self):
        self.context = compizconfig.Context ()

