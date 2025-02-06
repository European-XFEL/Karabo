from copy import deepcopy

import pytest

from karabo.native import AccessLevel
from karabogui.binding.api import (
    DeviceProxy, ProxyStatus, apply_default_configuration, build_binding)
from karabogui.dialogs.compare_device_configurations import (
    CompareDeviceConfigurationsDialog, DeviceSelectorDialog)
from karabogui.testing import (
    get_device_schema, get_device_schema_allowed_state, singletons,
    system_hash)
from karabogui.topology.api import SystemTopology


@pytest.fixture()
def dialog(gui_app):
    binding1 = build_binding(get_device_schema_allowed_state())
    proxy1 = DeviceProxy(binding=binding1, server_id="Test",
                         status=ProxyStatus.ONLINE)
    apply_default_configuration(proxy1.binding)

    binding2 = build_binding(get_device_schema())
    proxy2 = DeviceProxy(binding=binding2, server_id="Test",
                         status=ProxyStatus.ONLINE)
    apply_default_configuration(proxy2.binding)

    dialog = CompareDeviceConfigurationsDialog(proxy1, proxy2)
    yield dialog


def test_selector_dialog(gui_app):
    topology = SystemTopology()
    h = deepcopy(system_hash())
    h["device.davyy"] = None
    h["device.davyy", ...] = {
        "host": "BIG_IRON",
        "archive": True,
        "visibility": AccessLevel.OBSERVER,
        "type": "device",
        "capabilities": 0,
        "serverId": "otherserver",
        "classId": "FooClassCopy",
        "status": "ok",
        "interfaces": 0,
    }
    topology.initialize(h)
    with singletons(topology=topology):
        dialog = DeviceSelectorDialog("divvy")
        assert dialog.devices_combobox.count() == 1
        assert dialog.devices_combobox.currentText() == "davyy"


def test_compare_config_dialog(dialog):
    dialog._show_changes = False
    dialog._update_text()
    config1 = ("\nstate\nON"
               "\nstringProperty\nfoo"
               "\nreadOnlyProperty\n0"
               "\navailableScenes\n['scene']"
               "\nintProperty\n10"
               "\ninitOnlyString\nKarabo\n")
    assert dialog.reference_device_config.toPlainText() == config1

    config2 = ("\nstate\nON"
               "\nstringProperty\nfoo"
               "\nreadOnlyProperty\n0"
               "\navailableScenes\n['scene']\n")
    assert dialog.target_device_config.toPlainText() == config2

    # Show only reconfigurable properties.
    dialog.ui_show_only_reconfigurable.setChecked(True)
    dialog._update_text()

    reconfigurable1 = ("\nstate\nON"
                       "\nstringProperty\nfoo"
                       "\navailableScenes\n['scene']"
                       "\nintProperty\n10\n")
    assert dialog.reference_device_config.toPlainText() == reconfigurable1

    reconfigurable2 = ("\nstate\nON"
                       "\nstringProperty\nfoo"
                       "\navailableScenes\n['scene']\n")
    assert dialog.target_device_config.toPlainText() == reconfigurable2


def test_show_only_changes(dialog):
    assert dialog._show_changes
    config1 = ("\ninitOnlyString\nKarabo"
               "\nintProperty\n10\n")

    assert dialog.reference_device_config.toPlainText() == config1

    config2 = ("\ninitOnlyString""\nRemoved from configuration"
               "\nintProperty\nRemoved from configuration\n")

    assert dialog.target_device_config.toPlainText() == config2

    # Show only reconfigurable properties.
    dialog.ui_show_only_reconfigurable.setChecked(True)
    dialog._update_text()

    reconfigurable1 = ("\nintProperty\n10\n")
    assert dialog.reference_device_config.toPlainText() == reconfigurable1

    reconfigurable2 = ("\nintProperty\nRemoved from configuration\n")
    assert dialog.target_device_config.toPlainText() == reconfigurable2
