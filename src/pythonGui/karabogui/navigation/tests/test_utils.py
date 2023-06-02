from qtpy.QtGui import QIcon

from karabogui.navigation.utils import get_language_icon
from karabogui.topology.system_tree import SystemTreeNode


def test_get_language_icon(gui_app):
    """Ensure  get_language_icon returns the correct icon type."""
    node = SystemTreeNode()
    node.attributes = {"lang": "python"}
    icon = get_language_icon(node)
    assert isinstance(icon, QIcon)
