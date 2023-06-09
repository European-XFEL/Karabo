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
