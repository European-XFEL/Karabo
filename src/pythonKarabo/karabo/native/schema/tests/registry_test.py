# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest.mock import Mock

from karabo.native import Registry


def test_registry_metaclass():
    mock = Mock()

    class Foo(Registry):
        @classmethod
        def register(cls, name, dict):
            nonlocal mock
            mock(name)

    class Bar(Foo):
        pass

    mock.assert_called_with('Bar')
