from qtpy import uic
from qtpy.QtCore import QObject, Qt, Slot
from qtpy.QtWidgets import QCompleter, QDialog, QLineEdit
from traits.api import Undefined

from karabo.native import AccessMode, Hash, create_html_hash
from karabogui import icons
from karabogui.binding.api import (
    BindingRoot, DeviceProxy, ProjectDeviceProxy, get_config_changes,
    iterate_binding)
from karabogui.singletons.api import get_topology
from karabogui.topology.api import getTopology
from karabogui.util import SignalBlocker

from .utils import get_dialog_ui


def extract_configuration(binding):
    """Extract all the values set on a binding into a Hash object. Also
    copies the reconfigurable values into another Hash."""
    def _get_binding_value(binding):
        return binding.value

    all_config = Hash()
    reconfigurable = Hash()
    for key, node in iterate_binding(binding):
        value = _get_binding_value(node)
        if value is Undefined or isinstance(value, (bytes, bytearray)):
            # Skip pipeline data
            continue
        all_config[key] = value
        if node.accessMode == AccessMode.RECONFIGURABLE:
            reconfigurable[key] = value

    return all_config, reconfigurable


class CompareDeviceConfigurationsDialog(QDialog):
    """ Dialog to show the configurations of two devices, side by side."""
    def __init__(self, reference_proxy: ProjectDeviceProxy | DeviceProxy,
                 target_proxy: ProjectDeviceProxy | DeviceProxy,
                 parent: QObject = None):
        super().__init__(parent=parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        ui_file = get_dialog_ui("compare_configurations.ui")
        uic.loadUi(ui_file, self)
        self.reference_device.setText(reference_proxy.device_id)
        self.target_device.setText(target_proxy.device_id)
        self._show_reference_config(reference_proxy.binding)
        self._show_target_config(target_proxy.binding)
        self._load_differences()

        self.ui_synchronize_bars.toggled.connect(
            self._synchronize_toggled)
        self._synchronize_toggled(True)
        self.ui_show_only_reconfigurable.toggled.connect(
            self._show_only_reconfigurable)

        self._show_changes = False
        self.ui_swap_button.setIcon(icons.change)
        self.ui_swap_button.clicked.connect(self._show_only_changes)
        self._show_only_changes()

    def _show_reference_config(self, binding: BindingRoot) -> None:
        all_config, reconfigurable_config = extract_configuration(binding)
        self.reference_all_config = create_html_hash(all_config)
        self.reference_configurable_config = create_html_hash(
            reconfigurable_config)
        self.reference_all = all_config
        self.reference_reconfigurable = reconfigurable_config

    def _show_target_config(self, binding: BindingRoot) -> None:
        all_config, reconfigurable_config = extract_configuration(binding)
        self.target_all_config = create_html_hash(all_config)
        self.target_reconfigurable_config = create_html_hash(
            reconfigurable_config)
        self.target_all = all_config
        self.target_reconfigurable = reconfigurable_config

    def _load_differences(self) -> None:
        """Store the difference between the configuration - full
        configurations and reconfigurable configurations separately."""
        reference_all_diff, target_all_diff = get_config_changes(
            self.reference_all, self.target_all, False)
        self.reference_all_diff = create_html_hash(reference_all_diff)
        self.target_all_diff = create_html_hash(target_all_diff)

        ref_reconf_diff, target_reconf_diff = get_config_changes(
            self.reference_reconfigurable, self.target_reconfigurable, False)
        self.reference_reconfigurable_diff = create_html_hash(ref_reconf_diff)
        self.target_reconfigurable_diff = create_html_hash(target_reconf_diff)

    def _update_text(self) -> None:
        """Update the text edit with appropriate configuration depending on
        the state of options in the gui."""
        reference = self.reference_all_config
        target = self.target_all_config
        if self._show_changes:
            if self.ui_show_only_reconfigurable.isChecked():
                reference = self.reference_reconfigurable_diff
                target = self.target_reconfigurable_diff
            else:
                reference = self.reference_all_diff
                target = self.target_all_diff
        elif self.ui_show_only_reconfigurable.isChecked():
            reference = self.reference_configurable_config
            target = self.target_reconfigurable_config

        self.reference_device_config.setHtml(reference)
        self.target_device_config.setHtml(target)

    @Slot()
    def _show_only_changes(self) -> None:
        self._show_changes = not self._show_changes
        text = "Show Configuration" if self._show_changes else "Show Changes"
        self.ui_swap_button.setText(text)
        self._update_text()

    @Slot(bool)
    def _synchronize_toggled(self, toggled: bool) -> None:
        left_scrollbar = self.reference_device_config.verticalScrollBar()
        right_scrollbar = self.target_device_config.verticalScrollBar()
        if toggled:
            left_scrollbar.valueChanged.connect(self.on_left_scrollbar_moved)
            right_scrollbar.valueChanged.connect(
                self.on_right_scrollbar_moved)
        else:
            left_scrollbar.valueChanged.disconnect(
                self.on_left_scrollbar_moved)
            right_scrollbar.valueChanged.disconnect(
                self.on_right_scrollbar_moved)

    @Slot(int)
    def on_left_scrollbar_moved(self, value: int) -> None:
        with SignalBlocker(self.target_device_config):
            self.target_device_config.verticalScrollBar().setValue(value)

    @Slot(int)
    def on_right_scrollbar_moved(self, value: int) -> None:
        with SignalBlocker(self.reference_device_config):
            self.reference_device_config.verticalScrollBar().setValue(value)

    @Slot(bool)
    def _show_only_reconfigurable(self, toggled: bool) -> None:
        self._update_text()


class DeviceSelectorDialog(QDialog):
    """Dialog to choose a device from the list of matching devices with same
    classId."""
    def __init__(self, device_id: str, parent: QObject = None):
        super().__init__(parent=parent)
        uic.loadUi(get_dialog_ui("compare_configuration_launcher.ui"), self)
        self.device_id = device_id
        text = (
            f"Select a device to compare the configuration with <span style=' "
            f"font-style:italic;'>{device_id}</span>")
        self.info_label.setText(text)
        self._load_matching_devices(device_id)

    def _load_matching_devices(self, device_id: str) -> None:
        """Populate the matching devices with same class id. """
        topology = getTopology()
        attrs = topology[f"device.{device_id}", ...]
        class_id = attrs["classId"]
        matching_devices = []

        for deviceId, _, attrs in topology["device"].iterall():
            if deviceId == self.device_id:
                # Do not show the initial device.
                continue
            classId = attrs.get("classId")
            if classId in (class_id, f"{class_id}Copy"):
                matching_devices.append(deviceId)

        if not matching_devices:
            self.info_label.setText("No devices with the same classId are "
                                    "online")
            return

        self.devices_combobox.addItems(matching_devices)
        line_edit = QLineEdit()
        completer = QCompleter(matching_devices, parent=line_edit)
        completer.setCaseSensitivity(False)
        completer.setCompletionMode(QCompleter.PopupCompletion)
        line_edit.setCompleter(completer)
        self.devices_combobox.setLineEdit(line_edit)

    def accept(self) -> None:
        topology = get_topology()
        reference = topology.get_device(self.device_id)
        target = topology.get_device(self.devices_combobox.currentText())

        def config_update():
            target.on_trait_change(config_update, "config_update", remove=True)
            target.remove_monitor()
            dialog = CompareDeviceConfigurationsDialog(
                reference_proxy=reference, target_proxy=target,
                parent=self.parent())
            dialog.exec()

        target.on_trait_change(config_update, "config_update")
        target.add_monitor()
        super().accept()
