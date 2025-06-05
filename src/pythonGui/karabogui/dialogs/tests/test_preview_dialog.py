# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from contextlib import contextmanager
from unittest import mock

from karabo.native import Hash
from karabogui.binding.api import (
    BindingRoot, DeviceProxy, ProxyStatus, apply_default_configuration,
    build_binding)
from karabogui.dialogs.configuration_preview import (
    ConfigurationFromPastPreview)
from karabogui.singletons.configuration import Configuration
from karabogui.testing import (
    get_device_schema, get_device_schema_allowed_state, singletons)


def test_dialog(gui_app):
    configuration = Hash("stringProperty", "bar",
                         "intProperty", 3,
                         )
    config_singleton = Configuration()
    with singletons(configuration=config_singleton):
        binding = build_binding(get_device_schema_allowed_state())
        proxy = DeviceProxy(binding=binding, server_id="Test",
                            status=ProxyStatus.ONLINE)
        apply_default_configuration(proxy.binding)
        dialog = ConfigurationFromPastPreview("This is the title",
                                              "The information comes here",
                                              configuration=configuration,
                                              proxy=proxy)
        assert dialog.windowTitle() == "This is the title"
        assert dialog.ui_info.text() == ("The information comes here "
                                         "The device is online.")
        assert dialog.ui_config_count.text() == (
            "Showing 2 reconfigurable parameters and 1 are currently allowed.")
        assert dialog.ui_swap.text() == "Show Changes"
        dialog._swap_view()
        assert dialog.ui_swap.text() == "Show Configuration"
        assert not dialog.ui_hide_readonly.isChecked()
        assert dialog.ui_existing.toPlainText() == (
            "\navailableScenes\n['scene']\n"
            "initOnlyString\nKarabo\n"
            "intProperty\n10\n"
            "readOnlyProperty\n0\n"
            "state\nON\n"
            "stringProperty\nfoo\n")

        assert dialog.ui_retrieved.toPlainText() == (
            "\navailableScenes\nRemoved from configuration\n"
            "initOnlyString\nRemoved from configuration\n"
            "intProperty\n3\n"
            "readOnlyProperty\nRemoved from configuration\n"
            "state\nRemoved from configuration\n"
            "stringProperty\nbar\n")
        assert dialog.ui_text_info_all.toPlainText() == (
            "\nstringProperty\nbar\n"
            "intProperty\n3\n"
        )
        assert dialog.ui_text_info_configurable.toPlainText() == (
            "\nstringProperty\nbar\n"
            "intProperty\n3\n"
        )

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


def test_dialog_empty_configuration(gui_app):
    """Test the conflict with an empty configuration in saving"""
    configuration = Hash()
    config_singleton = Configuration()
    with singletons(configuration=config_singleton):
        binding = build_binding(get_device_schema())
        proxy = DeviceProxy(binding=binding, server_id="Test",
                            status=ProxyStatus.ONLINE)
        apply_default_configuration(proxy.binding)
        dialog = ConfigurationFromPastPreview("This is the title",
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


def test_dialog_empty_binding(gui_app):
    """Test the empty binding of the dialog protetion"""
    configuration = Hash()
    config_singleton = Configuration()
    with singletons(configuration=config_singleton):
        binding = BindingRoot()
        binding.class_id = "unknown-class"
        proxy = DeviceProxy(binding=binding, server_id="Test",
                            status=ProxyStatus.ONLINE)
        dialog = ConfigurationFromPastPreview("This is the title",
                                              "The information comes here",
                                              configuration=configuration,
                                              proxy=proxy)
        assert dialog.ui_existing.toPlainText() == (
            "No schema available to extract a configuration")
        assert dialog.ui_retrieved.toPlainText() == (
            "No schema available for comparing configurations!")


def test_dialog_only_reconfigurable_changes(gui_app):
    configuration = Hash("stringProperty", "bar",
                         "intProperty", 3,
                         "readOnlyProperty", 4,
                         )
    config_singleton = Configuration()
    with singletons(configuration=config_singleton):
        binding = build_binding(get_device_schema_allowed_state())
        proxy = DeviceProxy(binding=binding, server_id="Test",
                            status=ProxyStatus.ONLINE)
        apply_default_configuration(proxy.binding)
        dialog = ConfigurationFromPastPreview(
            "This is the title",
            "The information comes here",
            configuration=configuration,
            proxy=proxy)

        # Show all changes
        dialog._show_configuration_changes(hide_readonly=False)
        assert dialog.ui_existing.toPlainText() == (
            "\navailableScenes\n['scene']\n"
            "initOnlyString\nKarabo\n"
            "intProperty\n10\n"
            "readOnlyProperty\n0\n"
            "state\nON\n"
            "stringProperty\nfoo\n")
        assert dialog.ui_retrieved.toPlainText() == (
            "\navailableScenes\nRemoved from configuration\n"
            "initOnlyString\nRemoved from configuration\n"
            "intProperty\n3\n"
            "readOnlyProperty\n4\n"
            "state\nRemoved from configuration\n"
            "stringProperty\nbar\n")

        # Show only reconfigurable changes.
        dialog._show_configuration_changes(hide_readonly=True)
        assert dialog.ui_existing.toPlainText() == (
            "\navailableScenes\n['scene']\n"
            "initOnlyString\nKarabo\n"
            "intProperty\n10\n"
            "state\nON\n"
            "stringProperty\nfoo\n")

        assert dialog.ui_retrieved.toPlainText() == (
            "\navailableScenes\nRemoved from configuration\n"
            "initOnlyString\nRemoved from configuration\n"
            "intProperty\n3\n"
            "state\nRemoved from configuration\n"
            "stringProperty\nbar\n")
