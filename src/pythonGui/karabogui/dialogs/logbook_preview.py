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
from functools import partial

from qtpy import uic
from qtpy.QtCore import QBuffer, QByteArray, QIODevice, QSize, Qt, Signal, Slot
from qtpy.QtGui import QColor, QIcon, QPainter, QPixmap
from qtpy.QtWidgets import (
    QAction, QActionGroup, QCheckBox, QColorDialog, QDialog, QDialogButtonBox,
    QGraphicsScene, QLineEdit, QTableWidgetItem)

from karabo.common.scenemodel.api import create_base64image
from karabo.native import Hash
from karabogui import icons
from karabogui.const import IS_MAC_SYSTEM
from karabogui.dialogs.logbook_drawing_tools import BaseDrawingTool, get_tools
from karabogui.dialogs.utils import get_dialog_ui
from karabogui.events import (
    KaraboEvent, register_for_broadcasts, unregister_from_broadcasts)
from karabogui.singletons.api import get_config, get_network
from karabogui.validators import NumberValidator
from karabogui.widgets.toolbar import ToolBar

MIN_ZOOM_FACTOR = 25
MAX_ZOOM_FACTOR = 250
ZOOM_INCREMENT = 1.1

CHECKBOX_STYLE = """
QCheckBox {
    text-align: center;
    margin-left:50%;
    margin-right:50%;
}
"""

LOGBOOK_IMAGE = "Image"
LOGBOOK_DATA = "Data"


class Canvas(QGraphicsScene):
    """
    QGraphicsScene to draw shapes using mouse to annotate the image.
    """
    resetTool = Signal()

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.drawing_tool = None

    def mousePressEvent(self, event):
        if self._can_draw(event):
            self.drawing_tool.mouse_down(self, event)
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if self._can_draw(event):
            self.drawing_tool.mouse_move(self, event)
        super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event):
        if self._can_draw(event):
            self.drawing_tool.mouse_up(self, event)
        self.set_drawing_tool(None)
        super().mouseReleaseEvent(event)

    def _can_draw(self, event):
        return (self.drawing_tool is not None and
                self.sceneRect().contains(event.scenePos()))

    def set_drawing_tool(self, tool):
        """
        Set a drawing tool. Pass None to clear the active drawing tool.
        """
        assert tool is None or isinstance(tool, BaseDrawingTool)
        self.drawing_tool = tool
        if tool is None:
            self.resetTool.emit()

    def set_drawing_color(self, color):
        if self.drawing_tool is not None:
            self.drawing_tool.set_pen_color(color)


class LogBookPreview(QDialog):
    """A Dialog to preview the data to the LogBook"""

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.setModal(False)
        uic.loadUi(get_dialog_ui("logbook.ui"), self)

        self.action_group_draw = {}
        self._topics = {}

        self.pixmap = parent.grab()
        self.combo_datatype.addItem(LOGBOOK_IMAGE)
        if parent.info() is not None:
            self.combo_datatype.addItem(LOGBOOK_DATA)

        self.combo_datatype.setCurrentIndex(0)
        self.combo_datatype.currentIndexChanged.connect(
            self._on_datatype_changed)

        self.ok_button = self.buttonBox.button(QDialogButtonBox.Ok)
        self.ok_button.setText("Save")
        self.ok_button.setEnabled(False)

        self.canvas = Canvas(self)
        self.canvas.addPixmap(self.pixmap)
        self.view.setScene(self.canvas)
        self.canvas.resetTool.connect(self.reset_tool)

        validator = NumberValidator(
            minInc=MIN_ZOOM_FACTOR, maxInc=MAX_ZOOM_FACTOR, decimals=1)
        self.zoom_factor_edit = QLineEdit()
        self.zoom_factor_edit.setValidator(validator)
        self.zoom_factor.setLineEdit(self.zoom_factor_edit)

        self._fit_in_view()

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

        self.checkboxes = []
        self._load_table()
        self.select_all.clicked.connect(self._select_all)
        self.deselect_all.clicked.connect(self._deselect_all)

        self._annotation_color = QColor("red")
        self.draw_button.setIcon(icons.draw)
        self._create_zoom_toolbar()
        self.drawing_toolbar = self._create_drawing_toolbar()
        self.draw_button.toggled.connect(self._set_drawing_mode)
        self.drawing_toolbar.setVisible(False)

        title = repr(self.parent())
        self.title_line_edit.setPlaceholderText(title)

        self.combo_name.currentTextChanged.connect(self._update_topics)

        self.combo_title_style.currentTextChanged.connect(self._show_title)

    # -----------------------------------------------------------------------
    # Karabo Events

    def _event_destinations(self, data):
        """Show the available logbooks for the instrument, in a combobox.

        :param data: A HashList with available LogBooks: Hash with the
            following fields:
                "name" : Logbook identifier
                "destination": Url or folder of the logbook.
        """
        for index, proposal in enumerate(data):
            name = proposal.get("name")
            destination = proposal.get("destination")
            self._topics[name] = proposal.get("topics")
            self.combo_name.addItem(name)
            self.combo_name.setItemData(index, destination)
        self.ok_button.setEnabled(bool(data))

        previous_stream = get_config()["logbook_stream"]
        self.combo_name.setCurrentText(previous_stream)

        previous_topic = get_config()["logbook_topic"]
        self.combo_topic.setCurrentText(previous_topic)

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

    @Slot()
    def _select_all(self):
        for checkbox in self.checkboxes:
            checkbox.setChecked(True)
        self.ok_button.setEnabled(bool(self.checkboxes))

    @Slot()
    def _deselect_all(self):
        for checkbox in self.checkboxes:
            checkbox.setChecked(False)
        self.ok_button.setEnabled(False)

    @Slot(int)
    def _on_datatype_changed(self, index):
        self.stackedWidget.setCurrentIndex(index)
        self._update_save_button()

    @Slot()
    def _update_save_button(self):
        """Enable the Save button when at least one of the property is
        selected and disable, otherwise.
        For image preview , it is always enabled."""
        enabled = self.stackedWidget.currentWidget() == self.image_page
        if not enabled:
            enabled = any(checkbox.isChecked() for checkbox in self.checkboxes)
        self.ok_button.setEnabled(enabled)

    @Slot(bool)
    def _set_drawing_mode(self, enabled):
        self.drawing_toolbar.setVisible(enabled)
        if not enabled:
            self.canvas.set_drawing_tool(None)

    @Slot(str)
    def _update_topics(self, text):
        self.combo_topic.clear()
        topics = self._topics[text]
        self.combo_topic.addItems(topics)

    @Slot(str)
    def _show_title(self, text):
        """Do not show the title widget when 'No title' option is selected """
        index = 0 if text == "No title" else 1
        self.title_stackedWidget.setCurrentIndex(index)
    # -----------------------------------------------------------------------
    # Internal Interface

    def _load_table(self):
        data = self.parent().info()
        if data is None:
            return

        self.table.setColumnCount(2)
        self.table.setHorizontalHeaderLabels(["Select", "Property"])
        self.table.setSelectionBehavior(self.table.SelectRows)

        row = 0
        for (k, v, _) in Hash.flat_iterall(data, empty=False):
            if isinstance(v, (bytes, bytearray)):
                continue
            self.table.insertRow(row)

            checkbox = QCheckBox()
            checkbox.setChecked(True)
            checkbox.setStyleSheet(CHECKBOX_STYLE)
            checkbox.toggled.connect(self._update_save_button)
            self.checkboxes.append(checkbox)
            self.table.setCellWidget(row, 0, checkbox)

            item = QTableWidgetItem(k)
            flags = item.flags()
            flags &= ~ Qt.ItemIsEditable
            item.setFlags(flags)
            item.setData(Qt.UserRole, v)
            self.table.setItem(row, 1, item)

            row += 1

    def _scale_image(self, factor):
        self.view.resetTransform()
        self.view.scale(factor, factor)

    def _extract_panel_image(self):
        image_byte = QByteArray()
        io_buffer = QBuffer(image_byte)
        io_buffer.open(QIODevice.WriteOnly)
        image_format = "png"
        pixmap = QPixmap(self.pixmap.size())
        with QPainter(pixmap) as painter:
            self.canvas.render(painter, source=self.canvas.sceneRect(),
                               mode=Qt.KeepAspectRatio)
        pixmap.save(io_buffer, image_format)
        return {"data": create_base64image(image_format, image_byte),
                "dataType": "text_image",
                "caption": self.caption_edit.toPlainText()}

    def _extract_panel_data(self):
        h = Hash()
        for row, checkbox in enumerate(self.checkboxes):
            if checkbox.isChecked():
                item = self.table.item(row, 1)
                name = item.data(Qt.DisplayRole)
                value = item.data(Qt.UserRole)
                h[name] = value
        return {"data": h, "dataType": "hash"}

    def _create_logbook_data(self):
        panel = self.parent()
        stack = self.combo_datatype.currentText()
        info = {}
        header_type = "none"

        title = self.title_line_edit.text().strip()
        if not title:
            title = repr(panel)

        if self.combo_title_style.currentText() != "No title":
            header_type = self.combo_title_style.currentText().lower()

        if stack == LOGBOOK_DATA:
            info["title"] = f"Data: {title}"
            info.update(self._extract_panel_data())
        elif stack == LOGBOOK_IMAGE:
            info["title"] = f"Image: {title}"
            info.update(self._extract_panel_image())

        info["headerType"] = header_type

        return info

    def _fit_in_view(self):
        """Adjust the image's size to fit the QGraphicsView, ensuring the
        entire image is visible."""
        scene_rect = self.view.sceneRect()
        self.view.fitInView(scene_rect, Qt.KeepAspectRatio)

    # -----------------------------------------------------------------------
    # Public interface

    def done(self, result):
        if result:
            self.request_save()
            get_config()["logbook_stream"] = self.combo_name.currentText()
            get_config()["logbook_topic"] = self.combo_topic.currentText()
        unregister_from_broadcasts(self._event_map)
        return super().done(result)

    def request_save(self):
        """Request saving a Hash to the KaraboLogBook"""
        logbook = self.combo_name.currentText()
        topic = self.combo_topic.currentText()
        info = self._create_logbook_data()
        get_network().onSaveLogBook(name=logbook, topic=topic, **info)

    def _create_zoom_toolbar(self):
        """ Create Toolbar with zooming options"""

        zoom_toolbar = ToolBar(parent=self)
        zoom_in_action = QAction(icons.zoomIn, "Zoom In", self)
        zoom_in_action.triggered.connect(self.zoom_in)

        zoom_out_action = QAction(icons.zoomOut, "Zoom Out", self)
        zoom_out_action.triggered.connect(self.zoom_out)
        zoom_toolbar.addAction(zoom_in_action)
        zoom_toolbar.addAction(zoom_out_action)
        size = QSize(23, 23)
        zoom_toolbar.setIconSize(size)
        zoom_toolbar.add_expander()
        self.toolbar_layout.insertWidget(0, zoom_toolbar)

    def _create_drawing_toolbar(self):
        """Create a toolbar with annotate tools """
        toolbar = ToolBar(parent=self)
        self.action_group_draw = QActionGroup(self)

        color_action = QAction(parent=self)
        color_action.setToolTip("Change annotation color")
        color_action.triggered.connect(self._change_color)
        toolbar.addAction(color_action)
        self.action_group_draw.addAction(color_action)
        self._color_action = color_action
        self._set_button_color()

        for tool_factory in get_tools():
            text = tool_factory.name
            action = QAction(tool_factory.icon, text, self)
            tooltip = f"Add {text} to the Image"
            if text == "Eraser":
                tooltip = "Delete item"
            action.setToolTip(tooltip)
            action.setCheckable(True)
            tool = tool_factory.drawing_tool()
            action.triggered.connect(partial(self.activate_tool, tool))
            toolbar.addAction(action)
            self.action_group_draw.addAction(action)

        self.action_group_draw.setExclusive(True)
        self.toolbar_layout.insertWidget(1, toolbar)
        return toolbar

    @Slot()
    def _change_color(self):
        """Show a color dialog to choose the annotation color """
        color = QColorDialog.getColor(
            initial=self._annotation_color, parent=self)
        # To avoid the dialog hiding behind the main window
        if IS_MAC_SYSTEM:
            self.raise_()
        if color.isValid():
            self._annotation_color = color
            self._set_button_color()
            self.canvas.set_drawing_color(color)

    def _set_button_color(self):
        """Set the color on the color button."""
        pixmap = QPixmap(16, 16)
        pixmap.fill(self._annotation_color)
        self._color_action.setIcon(QIcon(pixmap))

    @Slot()
    def activate_tool(self, tool):
        self.view.setCursor(Qt.CrossCursor)
        self.canvas.set_drawing_tool(tool)
        self.canvas.set_drawing_color(self._annotation_color)

    @Slot()
    def reset_tool(self):
        for action in self.action_group_draw.actions():
            action.setChecked(False)
        self.view.setCursor(Qt.ArrowCursor)
