from qtpy.QtCore import QItemSelectionModel

from karabogui.testing import click_button, singletons, system_hash
from karabogui.topology.api import SystemTopology

from ..table_device_dialog import TableDeviceDialog


def test_device_dialog(gui_app):
    topology = SystemTopology()
    topology.initialize(system_hash())
    with singletons(topology=topology):
        dialog = TableDeviceDialog()
        assert dialog.device_id == ""
        assert dialog.filter_model.rowCount() == 2
        selection = dialog.ui_devices_view.selectionModel()
        model = dialog.filter_model
        index = model.index(0, 0)
        selection.setCurrentIndex(index, QItemSelectionModel.ClearAndSelect)
        dialog.done(1)
        assert dialog.device_id == "divvy"

        dialog.filter_model.setFilterFixedString("nooo")
        assert dialog.filter_model.rowCount() == 0
        # Clear to get previous selection
        click_button(dialog.ui_clear)
        assert dialog.filter_model.rowCount() == 2
        dialog.ui_interface.setCurrentIndex(1)
        # Interface, all filtered
        assert dialog.filter_model.rowCount() == 0
        click_button(dialog.ui_clear)
        assert dialog.filter_model.rowCount() == 2