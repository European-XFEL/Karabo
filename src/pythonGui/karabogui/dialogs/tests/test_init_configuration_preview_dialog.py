import numpy as np

from karabo.native import (
    Assignment, Bool, Configurable, Hash, Int8, Node, Slot, String)
from karabogui.binding.api import (
    DeviceProxy, apply_default_configuration, build_binding)
from karabogui.dialogs.init_configuration_preview import (
    InitConfigurationPreviewDialog, extract_defaults)
from karabogui.testing import singletons
from karabogui.topology.api import SystemTopology


class ANode(Configurable):
    name = String(defaultValue="A Node")
    integer = Int8(defaultValue=1)
    flag = Bool(defaultValue=True)


class Object(Configurable):
    val = String(defaultValue="FooString")
    intValue = Int8(defaultValue=10)
    intNoDefault = Int8()
    aNode = Node(ANode, displayedName="TestNode")
    internal = Int8(defaultValue=1, assignment=Assignment.INTERNAL)

    @Slot()
    async def dummySlot(self):
        pass


def get_default_values():
    node = Hash("name", "A Node", "integer", np.int8(1), "flag", True)
    return Hash("val", "FooString", "intValue", np.int8(10),
                "aNode", node)


init_config = """
val
New text
aNode

integer
20
"""

init_config_with_defaults = """
val
New text
intValue
10
aNode

name
A Node
integer
20
flag
True
"""


def test_extract_default_value_configuration():
    schema = Object.getClassSchema()
    binding = build_binding(schema)
    apply_default_configuration(binding)
    expected = get_default_values()
    default_values = extract_defaults(binding)
    assert default_values == expected


def test_init_configuration_preview_dialog(gui_app):
    """Test the InitConfigurationPreviewDialog with a sample configuration."""
    schema = Object.getClassSchema()
    binding = build_binding(schema)
    apply_default_configuration(binding)
    proxy = DeviceProxy(binding=binding)
    proxy.server_id = "swerver"

    node = Hash("integer", np.int8(20))
    config = Hash("val", "New text", "aNode", node)

    topology = SystemTopology()
    topology._class_proxies = {
        (proxy.server_id, proxy.binding.class_id): proxy}
    with singletons(topology=topology):
        dialog = InitConfigurationPreviewDialog(proxy, config,
                                                title="Test Dialog")
        assert dialog.ui_configuration_text.toPlainText() == init_config
        assert dialog.ui_full_configuration_text.toPlainText() == (
            init_config_with_defaults)

        # Verify the configuration to apply.
        assert dialog.configuration_to_apply == config
        dialog.ui_configuration_selector.setCurrentIndex(1)
        full_config = extract_defaults(binding)
        full_config.merge(config)
        assert dialog.configuration_to_apply == full_config
