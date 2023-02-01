from contextlib import contextmanager
from unittest import main, mock

from karabo.native import Hash
from karabogui.binding.api import (
    BindingRoot, DeviceProxy, ProxyStatus, apply_default_configuration,
    build_binding)
from karabogui.dialogs.configuration_preview import ConfigPreviewDialog
from karabogui.singletons.configuration import Configuration
from karabogui.testing import GuiTestCase, get_device_schema, singletons


class TestPreviewDialog(GuiTestCase):

    def test_dialog(self):
        configuration = Hash("stringProperty", "bar")
        config_singleton = Configuration()
        with singletons(configuration=config_singleton):
            binding = build_binding(get_device_schema())
            proxy = DeviceProxy(binding=binding, server_id="Test",
                                status=ProxyStatus.ONLINE)
            apply_default_configuration(proxy.binding)
            dialog = ConfigPreviewDialog("This is the title",
                                         "The information comes here",
                                         configuration=configuration,
                                         proxy=proxy)
            self.assertEqual(dialog.windowTitle(), "This is the title")
            self.assertEqual(dialog.ui_info.text(), "The information "
                                                    "comes here")

            self.assertEqual(dialog.ui_swap.text(), "Show changes")
            dialog._swap_view()
            self.assertEqual(dialog.ui_swap.text(), "Show Configuration")
            self.assertEqual(dialog.ui_existing.toPlainText(),
                             "\navailableScenes\n['scene']\n"
                             "readOnlyProperty\n0\n"
                             "state\nON\n"
                             "stringProperty\nfoo\n")
            self.assertEqual(dialog.ui_retrieved.toPlainText(),
                             "\navailableScenes\nRemoved from configuration\n"
                             "readOnlyProperty\nRemoved from configuration\n"
                             "state\nRemoved from configuration\n"
                             "stringProperty\nbar\n")
            save_path = "karabogui.dialogs.configuration_preview." \
                        "getSaveFileName"
            open_path = "karabogui.dialogs.configuration_preview.open"
            write_xml = "karabogui.dialogs.configuration_preview.writeXML"

            @contextmanager
            def open_file(fn, mode):
                """A context manager to make sure we do not write something
                """
                yield fn

            with mock.patch(save_path) as save_dialog, \
                    mock.patch(open_path, new=open_file):
                with mock.patch(write_xml) as writer:
                    save_dialog.return_value = "ConfigPath"
                    dialog.save_configuration_to_file()
                    # Check first call
                    args = writer.call_args[0]
                    # First argument is a Hash with unknown class here
                    h = args[0]["unknown-class"]
                    assert h["stringProperty"] == "bar"

    def test_dialog_empty_configuration(self):
        """Test the conflict with an empty configuration in saving"""
        configuration = Hash()
        config_singleton = Configuration()
        with singletons(configuration=config_singleton):
            binding = build_binding(get_device_schema())
            proxy = DeviceProxy(binding=binding, server_id="Test",
                                status=ProxyStatus.ONLINE)
            apply_default_configuration(proxy.binding)
            dialog = ConfigPreviewDialog("This is the title",
                                         "The information comes here",
                                         configuration=configuration,
                                         proxy=proxy)

            save_path = "karabogui.dialogs.configuration_preview." \
                        "getSaveFileName"
            mbox = "karabogui.dialogs.configuration_preview.messagebox"
            with mock.patch(save_path) as save_dialog:
                with mock.patch(mbox) as mb:
                    save_dialog.return_value = "ConfigPath"
                    dialog.save_configuration_to_file()
                    mb.show_error.assert_called()

    def test_dialog_empty_binding(self):
        """Test the empty binding of the dialog protetion"""
        configuration = Hash()
        config_singleton = Configuration()
        with singletons(configuration=config_singleton):
            binding = BindingRoot()
            binding.class_id = "unknown-class"
            proxy = DeviceProxy(binding=binding, server_id="Test",
                                status=ProxyStatus.ONLINE)
            dialog = ConfigPreviewDialog("This is the title",
                                         "The information comes here",
                                         configuration=configuration,
                                         proxy=proxy)
            self.assertEqual(dialog.ui_existing.toPlainText(),
                             "No schema available to extract a configuration")
            self.assertEqual(dialog.ui_retrieved.toPlainText(),
                             "No schema available for comparison of "
                             "configurations!")


if __name__ == "__main__":
    main()
