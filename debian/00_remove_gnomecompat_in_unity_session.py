#!/usr/bin/python
# -*- coding: utf-8 -*-
# Copyright (C) 2014 Canonical
#
# Authors:
#  Marco Trevisan <marco.trevisan@canonical.com>
#  William Hua <william.hua@canonical.com>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; version 3.
#
# This program is distributed in the hope that it will be useful, but WITHOUTa
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

from gi.repository import Gio
import os,sys

COMPIZ_SCHEMA = "org.compiz"
COMPIZ_CORE_PATH = "/org/compiz/profiles/unity/plugins/core/"

if COMPIZ_SCHEMA not in Gio.Settings.list_schemas():
    print("No compiz schemas found, no migration needed")
    sys.exit(0)

core_settings = Gio.Settings(schema=COMPIZ_SCHEMA+".core", path=COMPIZ_CORE_PATH)
active_plugins = core_settings.get_strv("active-plugins")

if not "gnomecompat" in active_plugins:
    print("No gnomecompat plugin active, no migration needed")
    sys.exit(0)

try:
    active_plugins.remove("gnomecompat")
except ValueError:
    pass

# gsettings doesn't work directly, the key is somewhat reverted. Work one level under then: dconf!
# gsettings.set_strv("active-plugins", active_plugins)
from subprocess import Popen, PIPE, STDOUT
p = Popen(("dconf load "+COMPIZ_CORE_PATH).split(), stdout=PIPE, stdin=PIPE, stderr=STDOUT)
p.communicate(input="[/]\nactive-plugins={}".format(active_plugins).encode('utf-8'))
