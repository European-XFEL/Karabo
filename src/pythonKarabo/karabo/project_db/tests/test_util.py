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
from unittest import TestCase

from karabo.middlelayer import Configurable, Node

from ..bases import DatabaseBase
from ..util import DEFAULT_PROTOCOL, get_node, get_project


class TestPluginLoading(TestCase):
    def test_node(self):
        node = get_node()
        self.assertTrue(isinstance(node, Node))

    def test_project_default(self):
        class Manager(Configurable):
            node = get_node()

        manager = Manager()
        project = get_project(manager.node)
        self.assertTrue(isinstance(project, DatabaseBase))
        self.assertEqual(manager.node.protocol, DEFAULT_PROTOCOL)
        self.assertFalse(manager.node.testMode)
