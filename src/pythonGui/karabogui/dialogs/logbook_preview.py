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
# or FITNESS FOR A PARTICULAR PURPOSE.\

from enum import Enum

from qtpy import uic
from qtpy.QtCore import QBuffer, QByteArray, QIODevice, QSize, Slot
from qtpy.QtWidgets import QDialog, QDialogButtonBox, QGraphicsScene, QLineEdit

from karabo.common.scenemodel.api import create_base64image
from karabo.native import Hash
from karabogui import icons
from karabogui.dialogs.utils import get_dialog_ui
from karabogui.events import (
    KaraboEvent, register_for_broadcasts, unregister_from_broadcasts)
from karabogui.singletons.api import get_network
from karabogui.validators import NumberValidator

MIN_ZOOM_FACTOR = 25
MAX_ZOOM_FACTOR = 250
ZOOM_INCREMENT = 1.1

TOOLBUTTON_STYLE = """
QToolButton {
    border: none;
}
QToolButton:hover {
    border: 1px solid gray;
}
"""


class LogBookType(Enum):
    Image = "image"
    Table = "table"


class LogBookPreview(QDialog):
    """A Dialog to preview the data to the LogBook"""

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.setModal(False)
        uic.loadUi(get_dialog_ui("logbook.ui"), self)

        self.pixmap = parent.grab()
        for mode in LogBookType:
            self.combo_datatype.addItem(mode.name, mode)

        self.combo_datatype.setCurrentIndex(0)
        ok_button = self.buttonBox.button(QDialogButtonBox.Ok)
        ok_button.setText("Save")

        self._event_map = {
            KaraboEvent.ActiveDestinations: self._event_destinations}
        register_for_broadcasts(self._event_map)
        get_network().listDestinations()

        self.scene = QGraphicsScene()
        self.scene.addPixmap(self.pixmap)
        self.image_view.setScene(self.scene)

        self.zoom_factor.setCurrentText("100")
        self.zoom_factor.currentIndexChanged.connect(self.change_zoom_factor)

        validator = NumberValidator(
            minInc=MIN_ZOOM_FACTOR, maxInc=MAX_ZOOM_FACTOR, decimals=1)
        self.zoom_factor_edit = QLineEdit()
        self.zoom_factor_edit.setValidator(validator)
        self.zoom_factor_edit.editingFinished.connect(self.change_zoom_factor)

        self.zoom_factor.setLineEdit(self.zoom_factor_edit)

        self.zoom_in_button.clicked.connect(self.zoom_in)
        self.zoom_out_button.clicked.connect(self.zoom_out)
        self.zoom_in_button.setIcon(icons.zoomIn)
        self.zoom_out_button.setIcon(icons.zoomOut)

        self.zoom_in_button.setStyleSheet(TOOLBUTTON_STYLE)
        self.zoom_out_button.setStyleSheet(TOOLBUTTON_STYLE)

        size = QSize(23, 23)
        self.zoom_in_button.setIconSize(size)
        self.zoom_out_button.setIconSize(size)

        self.combo_datatype.currentIndexChanged.connect(
            self.stackedWidget.setCurrentIndex)

    @Slot()
    def zoom_in(self):
        value = float(self.zoom_factor_edit.text()) * ZOOM_INCREMENT
        self._apply_zoom(value)

    @Slot()
    def zoom_out(self):
        value = float(self.zoom_factor_edit.text()) * (1/ZOOM_INCREMENT)
        self._apply_zoom(value)

    def _apply_zoom(self, value):
        if not MIN_ZOOM_FACTOR < value < MAX_ZOOM_FACTOR:
            return
        factor = value / 100
        self._scale_image(factor)
        value = round(value, 1)
        self.zoom_factor_edit.setText(str(value))

    @Slot()
    def change_zoom_factor(self):
        value = self.zoom_factor_edit.text()
        self.zoom_factor_edit.text()

        factor = float(value) / 100
        self._scale_image(factor=factor)

    def _scale_image(self, factor):
        self.image_view.resetTransform()
        self.image_view.scale(factor, factor)

    def _event_destinations(self, data):
        """Show the available logbooks for the instrument, in a combobox.

        :param data: A Hash with information about available LogBooks,
            as the following fields:
                "name" : Logbook identifier
                "destination": Url or folder of the logbook.
        """
        for index, proposal in enumerate(data):
            name = proposal.get("name")
            destination = proposal.get("destination")
            self.combo_name.addItem(name)
            self.combo_name.setItemData(index, destination)

    def _extract_image_data(self):
        """Convert the QPixmap to an embedded href format."""
        pixmap = self.pixmap
        image_byte = QByteArray()
        buffer = QBuffer(image_byte)
        buffer.open(QIODevice.WriteOnly)
        pixmap.save(buffer, "PNG")
        return create_base64image("png", image_byte)

    def _extract_panel_info(self):
        """Extract the panel data"""
        widget = self.parent()
        data = widget.get_panel_info()
        if data is None:
            data = Hash()

        return data

    def _get_logbook_data(self):
        dataType = self.combo_datatype.currentData()
        if dataType is LogBookType.Table:
            return self._extract_panel_info()
        elif dataType is LogBookType.Image:
            return self._extract_image_data()

    def done(self, result):
        if result:
            self.request_save()
        unregister_from_broadcasts(self._event_map)
        return super().done(result)

    def request_save(self):
        """Request saving a Hash to the KaraboLogBook"""
        name = self.combo_name.currentText()
        name = name.split("_")[0]
        dataType = self.combo_datatype.currentData().value
        data = self._get_logbook_data()
        caption = self.caption_edit.toPlainText()
        get_network().onSaveLogBook(
            name=name, dataType=dataType, data=data,
            caption=caption)
