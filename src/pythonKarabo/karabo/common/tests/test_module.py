# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import sys
from types import ModuleType

from pytest import raises as assert_raises

from karabo.common.api import create_module


def test_create_module():
    def local_func():
        pass

    name = "whoa.crazy.new.module"

    assert name not in sys.modules

    module = create_module(name, sys, local_func)

    assert name in sys.modules
    assert isinstance(module, ModuleType)
    assert isinstance(module.sys, ModuleType)
    assert module.local_func is local_func


def test_create_module_overwrite():
    with assert_raises(AssertionError):
        create_module("sys", sys.path)
