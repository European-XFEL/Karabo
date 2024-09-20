from qtpy import uic
from qtpy.QtCore import QObject, Qt, Slot
from qtpy.QtWidgets import QCompleter, QDialog, QLineEdit
from traits.api import Undefined

from karabo.native import AccessMode, Hash, create_html_hash
from karabogui.binding.api import (
    BindingRoot, DeviceProxy, ProjectDeviceProxy, iterate_binding)
from karabogui.singletons.api import get_topology
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
        if node.access_mode == AccessMode.RECONFIGURABLE:
            reconfigurable[key] = value

    return all_config, reconfigurable


class CompareDeviceConfigurationsDialog(QDialog):
    """ Dilaog to show the configurations of two devices, side by side."""
    def __init__(self, reference_proxy: ProjectDeviceProxy | DeviceProxy,
                 target_proxy: ProjectDeviceProxy | DeviceProxy,
                 parent: QObject = None):
        super().__init__(parent=parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        ui_file = get_dialog_ui("compare_configuarations.ui")
        uic.loadUi(ui_file, self)
        self.reference_device.setText(reference_proxy.device_id)
        self.target_device.setText(target_proxy.device_id)
        self._show_reference_config(reference_proxy.binding)
        self._show_target_config(target_proxy.binding)

        self.ui_synchronize_bars.toggled.connect(
            self._synchronize_toggled)
        self._synchronize_toggled(True)
        self.ui_show_only_reconfigurable.toggled.connect(
            self._show_only_reconfigurable)

    def _show_reference_config(self, binding: BindingRoot) -> None:
        all_config, reconfigurable_config = extract_configuration(binding)
        self.reference_all_config = create_html_hash(all_config)
        self.reference_configurable_config = create_html_hash(
            reconfigurable_config)
        self.reference_device_config.setHtml(self.reference_all_config)

    def _show_target_config(self, binding: BindingRoot) -> None:
        all_config, reconfigurable_config = extract_configuration(binding)
        self.target_all_config = create_html_hash(all_config)
        self.target_reconfigurable_config = create_html_hash(
            reconfigurable_config)
        self.target_device_config.setHtml(self.target_all_config)

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
        reference_text = (self.reference_configurable_config if toggled
                          else self.reference_all_config)
        target_text = (self.target_reconfigurable_config if toggled
                       else self.target_all_config)

        self.reference_device_config.setHtml(reference_text)
        self.target_device_config.setHtml(target_text)


class DeviceSelectorDialog(QDialog):
    """Dialog to choose a device from the list of matching devices with same
    classId."""
    def __init__(self, device_id: str, parent: QObject = None):
        super().__init__(parent=parent)
        uic.loadUi(get_dialog_ui("compare_configuraiton_launcher.ui"), self)
        self.device_id = device_id
        text = (
            f"Select a device to compare the configuration with <span style=' "
            f"font-style:italic;'>{device_id}</span>")
        self.info_label.setText(text)
        self._load_matching_devices(device_id)

    def _load_matching_devices(self, device_id: str) -> None:
        """Populate the matching devices with same class id. """
        topology = get_topology()
        reference = topology.get_device(device_id)
        class_id = reference.binding.class_id
        matching_devices = []

        def visitor(node):
            if node.node_id == device_id:
                # Do not show the initial device.
                return
            attributes = node.attributes
            if attributes.get("type") == "device":
                if attributes.get("classId") == class_id:
                    matching_devices.append(node.node_id)

        topology.visit_system_tree(visitor=visitor)

        if not matching_devices:
            self.info_label.setText("No devices with same classId are online")
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
