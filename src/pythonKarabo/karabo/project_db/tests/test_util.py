# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
