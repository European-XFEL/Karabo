from karabogui.binding.api import DeviceClassProxy, build_binding
from karabogui.dialogs.logbook_preview import LogBookPreview
from karabogui.panels.api import ConfigurationPanel
from karabogui.singletons.mediator import Mediator
from karabogui.testing import get_simple_schema, singletons


def test_logbook_preview(gui_app):
    """Test the logbook preview with a configuration panel"""
    mediator = Mediator()
    with singletons(mediator=mediator):
        panel = ConfigurationPanel()
        dialog = LogBookPreview(parent=panel)

        schema = get_simple_schema()
        binding = build_binding(schema)
        proxy = DeviceClassProxy(server_id="test_server", binding=binding)
        panel._show_configuration(proxy)
        dialog.done(0)
