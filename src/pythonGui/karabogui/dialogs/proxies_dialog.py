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
from qtpy import uic
from qtpy.QtCore import QStringListModel, Slot
from qtpy.QtWidgets import QDialog, QMessageBox

from karabogui import icons, messagebox

from .utils import get_dialog_ui


class ProxiesDialog(QDialog):
    """The basic dialog to remove additional proxies from a `container`
    """

    def __init__(self, container, parent=None):
        super().__init__(parent)
        self.setModal(False)
        ui_file = get_dialog_ui("proxies_dialog.ui")
        uic.loadUi(ui_file, self)

        self.widget_container = container
        controller = container.widget_controller
        self.ui_main_property.setText(controller.proxy.key)

        list_model = QStringListModel()
        paths = [proxy.key for proxy in controller.proxies[1:]]
        list_model.setStringList(paths)
        self._list_model = list_model

        self.ui_list_view.setModel(list_model)
        # Removal button
        self.ui_remove_button.clicked.connect(self._on_remove_clicked)
        self.ui_remove_button.setIcon(icons.delete)
        self._on_update_buttons()

    # ----------------------------------------------------------------------
    # Private interface

    def _on_update_buttons(self):
        """Keep the removal button in sync with the items"""
        has_items = self._list_model.rowCount() > 0
        self.ui_remove_button.setEnabled(has_items)

    # ----------------------------------------------------------------------
    # Slots

    @Slot()
    def _on_remove_clicked(self):
        index = self.ui_list_view.selectionModel().currentIndex()
        if not index.isValid():
            return

        controller = self.widget_container.widget_controller
        proxy = controller.proxies[index.row() + 1]
        prop = proxy.key
        ask = f"Are you sure you want to remove the property <b>{prop}</b>?"
        options = (QMessageBox.Yes | QMessageBox.No)
        reply = QMessageBox.question(self, "Remove Property", ask, options,
                                     QMessageBox.No)
        if reply == QMessageBox.No:
            return

        if not self.widget_container.remove_proxies([proxy]):
            messagebox.show_error(f"The removal of property {prop} is not "
                                  "supported by this controller.",
                                  parent=self.parent())
            return
        self._list_model.setStringList([proxy.key for proxy
                                        in controller.proxies[1:]])
        self._on_update_buttons()
