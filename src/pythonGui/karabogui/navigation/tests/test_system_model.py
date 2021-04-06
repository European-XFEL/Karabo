import unittest

from qtpy.QtCore import Qt

from karabogui.navigation.system_model import SystemTreeModel


class TestCase(unittest.TestCase):

    def test_drag_actions(self):
        tree_model = SystemTreeModel()
        self.assertEqual(tree_model.supportedDragActions(), Qt.CopyAction)
