# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import unittest
from warnings import catch_warnings, simplefilter

from karabo.bound import Hash, PluginLoader


class PluginLoader_TestCase(unittest.TestCase):

    def test_default_namespace(self):
        loader = PluginLoader.create("PythonPluginLoader", Hash())
        entrypoints = loader.update()
        assert len(entrypoints) > 0, str(entrypoints)

    def test_broken_namespace(self):
        loaderCfg = Hash("pluginNamespace", "karabo.bound_broken_device_test")
        loader = PluginLoader.create("PythonPluginLoader", loaderCfg)
        with catch_warnings(record=True) as warnings:
            simplefilter("always")
            # this will not cause an exception, but warnings will be raised
            entrypoints = loader.update()
            assert len(warnings) == 1, warnings
            assert issubclass(warnings[-1].category, ImportWarning)
            assert len(entrypoints) == 0
