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

from qtpy import uic
from qtpy.QtCore import QBuffer, QByteArray, QIODevice, QSize, Qt, Slot
from qtpy.QtWidgets import QDialog, QDialogButtonBox, QGraphicsScene, QLineEdit

from karabo.common.scenemodel.api import create_base64image
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

LOGBOOK_IMAGE = "Image"
LOGBOOK_DATA = "Data"


class LogBookPreview(QDialog):
    """A Dialog to preview the data to the LogBook"""

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.setModal(False)
        uic.loadUi(get_dialog_ui("logbook.ui"), self)

        self.pixmap = parent.grab()
        self.combo_datatype.addItem(LOGBOOK_IMAGE)
        if parent.info() is not None:
            self.combo_datatype.addItem(LOGBOOK_DATA)

        self.combo_datatype.setCurrentIndex(0)
        self.combo_datatype.currentIndexChanged.connect(
            self.stackedWidget.setCurrentIndex)

        self.ok_button = self.buttonBox.button(QDialogButtonBox.Ok)
        self.ok_button.setText("Save")
        self.ok_button.setEnabled(False)

        self.scene = QGraphicsScene()
        self.scene.addPixmap(self.pixmap)
        self.view.setScene(self.scene)

        validator = NumberValidator(
            minInc=MIN_ZOOM_FACTOR, maxInc=MAX_ZOOM_FACTOR, decimals=1)
        self.zoom_factor_edit = QLineEdit()
        self.zoom_factor_edit.setValidator(validator)
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

        # Adjust the image's size to fit the QGraphicsView, ensuring the
        # entire image is visible.
        scene_rect = self.view.sceneRect()
        self.view.fitInView(scene_rect, Qt.KeepAspectRatio)

        # Show the current zoom factor of the image.
        transform = self.view.transform()
        width_scale = transform.m11()
        height_scale = transform.m22()
        zooming_factor = ((width_scale + height_scale) / 2) * 100
        self.zoom_factor_edit.setText(str(round(zooming_factor, 1)))
        self.zoom_factor.currentIndexChanged.connect(self.change_zoom_factor)
        self.zoom_factor_edit.editingFinished.connect(self.change_zoom_factor)

        self._event_map = {
            KaraboEvent.ActiveDestinations: self._event_destinations}
        register_for_broadcasts(self._event_map)
        get_network().listDestinations()

    # -----------------------------------------------------------------------
    # Karabo Events

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
        self.ok_button.setDisabled(data.empty())

    # -----------------------------------------------------------------------
    # Qt Slots

    @Slot()
    def zoom_in(self):
        value = float(self.zoom_factor_edit.text()) * ZOOM_INCREMENT
        self._apply_zoom(value)

    @Slot()
    def zoom_out(self):
        value = float(self.zoom_factor_edit.text()) * (1 / ZOOM_INCREMENT)
        self._apply_zoom(value)

    @Slot()
    def change_zoom_factor(self):
        value = self.zoom_factor_edit.text()
        self.zoom_factor_edit.text()

        factor = float(value) / 100
        self._scale_image(factor=factor)

    def _apply_zoom(self, value):
        if not MIN_ZOOM_FACTOR < value < MAX_ZOOM_FACTOR:
            return
        factor = value / 100
        self._scale_image(factor)
        value = round(value, 1)
        self.zoom_factor_edit.setText(str(value))

    # -----------------------------------------------------------------------
    # Internal Interface

    def _scale_image(self, factor):
        self.view.resetTransform()
        self.view.scale(factor, factor)

    def _extract_panel_image(self):
        image_byte = QByteArray()
        io_buffer = QBuffer(image_byte)
        io_buffer.open(QIODevice.WriteOnly)
        image_format = "png"
        self.pixmap.save(io_buffer, image_format)
        return {"data": create_base64image(image_format, image_byte),
                "dataType": "image",
                "caption": self.caption_edit.toPlainText()}

    def _create_logbook_data(self):
        panel = self.parent()
        stack = self.combo_datatype.currentText()
        info = {}
        if stack == LOGBOOK_DATA:
            info["title"] = f"Data: {repr(panel)}"
            info.update(panel.info())
        elif stack == LOGBOOK_IMAGE:
            info["title"] = f"Image: {repr(panel)}"
            info.update(self._extract_panel_image())

        return info

    # -----------------------------------------------------------------------
    # Public interface

    def done(self, result):
        if result:
            self.request_save()
        unregister_from_broadcasts(self._event_map)
        return super().done(result)

    def request_save(self):
        """Request saving a Hash to the KaraboLogBook"""
        logbook = self.combo_name.currentText()
        info = self._create_logbook_data()
        get_network().onSaveLogBook(name=logbook, **info)
