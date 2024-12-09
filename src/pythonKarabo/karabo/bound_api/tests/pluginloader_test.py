# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import unittest

from karabo.bound import Hash, PluginLoader


class PluginLoader_TestCase(unittest.TestCase):

    def test_default_namespace(self):
        loader = PluginLoader.create("PythonPluginLoader", Hash())
        entrypoints = loader.update()
        assert len(entrypoints) > 0, str(entrypoints)

    def test_broken_namespace(self):
        loaderCfg = Hash("pluginNamespace", "karabo.bound_broken_device_test")
        loader = PluginLoader.create("PythonPluginLoader", loaderCfg)
        entrypoints = loader.update()
        assert len(entrypoints) == 0
        assert len(loader.plugin_errors) > 0
