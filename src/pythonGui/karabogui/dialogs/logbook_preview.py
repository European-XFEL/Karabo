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
    QAction, QActionGroup, QCheckBox, QColorDialog, QComboBox, QDialog,
    QDialogButtonBox, QGraphicsPixmapItem, QGraphicsScene, QGridLayout, QLabel,
    QLineEdit, QSizePolicy, QTableWidgetItem, QToolButton, QToolTip, QWidget)

from karabo.common.scenemodel.api import create_base64image
from karabo.native import Hash, HashList
from karabogui import icons
from karabogui.const import IS_MAC_SYSTEM
from karabogui.dialogs.logbook_drawing_tools import (
    BaseDrawingTool, CropTool, get_tools)
from karabogui.dialogs.utils import get_dialog_ui
from karabogui.events import (
    KaraboEvent, register_for_broadcasts, unregister_from_broadcasts)
from karabogui.singletons.api import get_config, get_network
from karabogui.util import SignalBlocker
from karabogui.validators import NumberValidator
from karabogui.widgets.toolbar import ToolBar

MIN_ZOOM_FACTOR = 25
MAX_ZOOM_FACTOR = 250
ZOOM_INCREMENT = 1.1

MAX_ALLOWED_LOGBOOK = 3

CHECKBOX_STYLE = """
QCheckBox {
    text-align: center;
    margin-left:50%;
    margin-right:50%;
}
"""

LOGBOOK_IMAGE = "Image"
LOGBOOK_DATA = "Data"

DESTINATION_TYPE_HINT = """<b>Active Proposals.</b><br>
<i>Ready Proposals</i>.<br>
Operational Logbooks."""


class DestinationWidget(QWidget):
    """A container  with widgets to select the Stream and Topic of the
    logbook """

    def __init__(self, parent=None):
        super().__init__(parent=parent)

        self._topics = {}
        layout = QGridLayout(self)
        layout.setSpacing(1)
        layout.setVerticalSpacing(2)
        layout.setContentsMargins(0, 0, 0, 0)
        self.combo_stream = QComboBox()
        self.combo_topic = QComboBox()

        self.create_topic = QToolButton()
        self.create_topic.setIcon(icons.edit)
        self.create_topic.setCheckable(True)

        self.combo_stream.currentTextChanged.connect(self._update_topics)
        self.create_topic.clicked.connect(self.allow_topic_creation)

        stream_label = QLabel("Stream")
        stream_label.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        topic_label = QLabel("Topic")
        topic_label.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)

        info_button = QToolButton()
        info_button.setIcon(icons.info)
        info_button.setToolTip(DESTINATION_TYPE_HINT)
        info_button.clicked.connect(self._show_info_popup)
        info_button.setStyleSheet("QToolButton{border:none}")

        layout.addWidget(stream_label, 0, 0)
        layout.addWidget(info_button, 0, 2)
        layout.addWidget(self.combo_stream, 0, 1)
        layout.addWidget(topic_label, 1, 0)
        layout.addWidget(self.combo_topic, 1, 1)
        layout.addWidget(self.create_topic, 1, 2)

    def update_destinations(self, data):
        """
        Update the comboboxes with the available destinations - stream/topic.

        :param data: A HashList with available LogBooks: Hash with the
            following fields:
                "name" : Logbook identifier
                "destination": Url or folder of the logbook.
                "topics": List of topics in the stream.
        """
        with SignalBlocker(self.combo_stream):
            self.combo_stream.clear()
        self._topics.clear()

        for index, proposal in enumerate(data):
            name = proposal.get("name")
            destination = proposal.get("destination")
            status = proposal.get("beamlineStatus")
            self._topics[name] = proposal.get("topics")
            self.combo_stream.addItem(name)
            self.combo_stream.setItemData(index, destination)
            font = self.combo_stream.font()
            if status == "Active":
                font.setBold(True)
            if status == "Ready":
                font.setItalic(True)
            self.combo_stream.setItemData(index, font, Qt.FontRole)

    def select_destination(self, stream: str, topic: str):
        self.combo_stream.setCurrentText(stream)
        self.combo_topic.setCurrentText(topic)

    @Slot()
    def _show_info_popup(self):
        button = self.sender()
        QToolTip.showText(button.mapToGlobal(button.rect().topLeft()),
                          button.toolTip(), button)

    @Slot(str)
    def _update_topics(self, text):
        """Update the combobox with available topics when the stream
        selection change"""
        self.combo_topic.clear()
        topics = self._topics[text]
        self.combo_topic.addItems(topics)

    @Slot(bool)
    def allow_topic_creation(self, toggled):
        """Make the combobox editable to allow creation of new topic"""
        if toggled:
            self.combo_topic.setEditable(True)
            self.combo_topic.lineEdit().selectAll()
            self.combo_topic.setFocus(True)
            self.combo_topic.lineEdit().editingFinished.connect(
                self.add_new_topic)
        else:
            new_topic = self.combo_topic.lineEdit().text()
            self._add_topic(new_topic)

    @Slot()
    def add_new_topic(self):
        """Add a new item to the Topic combobox"""
        new_topic = self.combo_topic.lineEdit().text()
        if not new_topic.strip():
            self.combo_topic.setEditable(False)
            self.create_topic.setChecked(False)
            return
        self._add_topic(new_topic)

    def _add_topic(self, new_topic):
        if self.combo_topic.findText(new_topic) == -1:
            self.combo_topic.addItem(new_topic)
        self.combo_topic.setEditable(False)
        self.combo_topic.setCurrentText(new_topic)
        self.create_topic.setChecked(False)

    @property
    def topic(self):
        return self.combo_topic.currentText()

    @property
    def stream(self):
        return self.combo_stream.currentText()


class Canvas(QGraphicsScene):
    """
    QGraphicsScene to draw shapes using mouse to annotate the image.
    """
    resetTool = Signal()
    fitViewRequested = Signal()

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        self.drawing_tool = None

    def mousePressEvent(self, event):
        if self._can_draw(event):
            self.drawing_tool.mouse_down(self, event)
        else:
            self.set_drawing_tool(None)
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if self._can_draw(event):
            self.drawing_tool.mouse_move(self, event)
        super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event):
        if self._can_draw(event):
            self.drawing_tool.mouse_up(self, event)
        elif isinstance(self.drawing_tool, CropTool):
            self.removeItem(self.drawing_tool.graphics_item)
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

    def crop(self, rect):
        for item in self.items():
            if isinstance(item, QGraphicsPixmapItem):
                break
            item = None
        if item is None:
            return
        rect = rect.normalized()
        height = int(rect.height())
        width = int(rect.width())
        pixmap = QPixmap(width, height)
        with QPainter(pixmap) as painter:
            self.render(painter, source=rect, mode=Qt.KeepAspectRatio)
        item.setPixmap(pixmap)
        self.setSceneRect(0, 0, rect.width(), rect.height())
        self.fitViewRequested.emit()

    @property
    def pixmap_item(self):
        for item in self.items():
            if isinstance(item, QGraphicsPixmapItem):
                return item.pixmap()


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
        self.canvas.fitViewRequested.connect(self._fit_in_view)

        validator = NumberValidator(
            minInc=MIN_ZOOM_FACTOR, maxInc=MAX_ZOOM_FACTOR, decimals=1)
        self.zoom_factor_edit = QLineEdit()
        self.zoom_factor_edit.setValidator(validator)
        self.zoom_factor.setLineEdit(self.zoom_factor_edit)

        self._fit_in_view()

        self.data = None
        self._destinations = []

        self.zoom_factor.currentIndexChanged.connect(self.change_zoom_factor)
        self.zoom_factor_edit.editingFinished.connect(self.change_zoom_factor)

        self._event_map = {
            KaraboEvent.ActiveDestinations: self._event_destinations,
            KaraboEvent.NetworkConnectStatus: self._event_network
        }
        register_for_broadcasts(self._event_map)
        get_network().listDestinations()

        self.checkboxes = []
        self._load_table()
        self.select_all.clicked.connect(self._select_all)
        self.deselect_all.clicked.connect(self._deselect_all)

        self._annotation_color = QColor("red")
        self._create_zoom_toolbar()
        self.drawing_toolbar = self._create_drawing_toolbar()

        title = repr(self.parent())
        self.title_line_edit.setPlaceholderText(title)

        self.combo_title_style.currentTextChanged.connect(self._show_title)
        style = get_config()["logbook_header_style"] or "No title"
        self.combo_title_style.setCurrentText(style)

        self.add_stream.clicked.connect(self.add_destination)
        self.remove_stream.clicked.connect(self.remove_destination)
        self.add_stream.setIcon(icons.plus)
        self.remove_stream.setIcon(icons.minus)

        self.add_destination()

    # -------------------------------------------------------------
    # Karabo Events

    def _event_destinations(self, data):
        """Show the available logbooks for the instrument, in a combobox.

        :param data: A HashList with available LogBooks: Hash with the
            following fields:
                "name" : Logbook identifier
                "destination": Url or folder of the logbook.
        """
        self.data = data

        previous_stream = get_config()["logbook_stream"]
        previous_topic = get_config()["logbook_topic"] or "Logbook"

        for widget in self._destinations:
            widget.update_destinations(data)
            widget.combo_stream.setCurrentText(previous_stream)
            widget.combo_topic.setCurrentText(previous_topic)

        self._update_save_button()

    def _event_network(self, data):
        if not data.get("status"):
            self.close()

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
        self._update_save_button()

    @Slot()
    def _deselect_all(self):
        for checkbox in self.checkboxes:
            checkbox.setChecked(False)
        self._update_save_button()

    @Slot(int)
    def _on_datatype_changed(self, index):
        self.stackedWidget.setCurrentIndex(index)
        self._update_save_button()

    @Slot()
    def _update_save_button(self):
        """Enable the Save button. The button is disabled if

        - no topic is provided
        - dataType is hash and not at least one property is selected
        """

        if not self._destinations:
            self.ok_button.setEnabled(False)
            return
        topic = self._destinations[0].topic
        if not topic:
            self.ok_button.setEnabled(False)
            return

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
    def _show_title(self, text):
        """Do not show the title widget when 'No title' option is selected """
        index = 0 if text == "No title" else 1
        self.title_stackedWidget.setCurrentIndex(index)

    @Slot()
    def add_destination(self):
        """Add a new logbook destination"""
        widget = self._create_destination_widget()
        if self.data is not None:
            widget.update_destinations(self.data)

        # Select the same stream/topic as in the default destination widget.
        destination = self._destinations[0]
        widget.select_destination(destination.stream, destination.topic)

        self.stream_widget.adjustSize()
        self._enable_destination_buttons()

    @Slot()
    def remove_destination(self):
        """Remove the last logbook destination"""
        widget = self._destinations[-1]
        self.destinations_layout.removeWidget(widget)
        widget.deleteLater()
        self.stream_widget.adjustSize()
        self._destinations.remove(widget)
        self._enable_destination_buttons()

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
        pixmap = self.canvas.pixmap_item
        if pixmap is None:
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
            info["uploadFile"] = self.upload_file.isChecked()
        elif stack == LOGBOOK_IMAGE:
            info["title"] = f"Image: {title}"
            info.update(self._extract_panel_image())

        info["headerType"] = header_type

        return info

    @Slot()
    def _fit_in_view(self):
        """Adjust the image's size to fit the QGraphicsView, ensuring the
        entire image is visible."""
        scene_rect = self.view.sceneRect()
        self.view.fitInView(scene_rect, Qt.KeepAspectRatio)

        # Show the current zoom factor of the image.
        transform = self.view.transform()
        width_scale = transform.m11()
        height_scale = transform.m22()
        zooming_factor = ((width_scale + height_scale) / 2) * 100
        self.zoom_factor_edit.setText(str(round(zooming_factor, 1)))

    def _create_destination_widget(self):
        destination_widget = DestinationWidget(parent=self.stream_widget)
        self.destinations_layout.addWidget(destination_widget)
        self._destinations.append(destination_widget)
        return destination_widget

    def _enable_destination_buttons(self):
        """Enable/Disable the Add/Remove Destination buttons- to allow min 1
        and max 3 destinations. """
        size = len(self._destinations)
        add_enabled = size < MAX_ALLOWED_LOGBOOK
        remove_enabled = size > 1
        self.add_stream.setEnabled(add_enabled)
        self.remove_stream.setEnabled(remove_enabled)

    # -----------------------------------------------------------------------
    # Public interface

    def done(self, result):
        if result:
            self.request_save()
            self._save_configs()
        unregister_from_broadcasts(self._event_map)
        return super().done(result)

    def request_save(self):
        """Request saving a Hash to the KaraboLogBook"""

        info = self._create_logbook_data()
        info["destinations"] = self._get_destinations()
        get_network().onSaveLogBook(**info)

    def _get_destinations(self):
        """Create a HashList from the selected Logbook stream and topic
        names in the GUI"""
        destinations = HashList()
        for widget in self._destinations:
            h = Hash("stream", widget.stream, "topic", widget.topic)
            destinations.append(h)
        return destinations

    def _save_configs(self):
        """Save the singleton configurations."""
        widget = self._destinations[0]
        get_config()["logbook_stream"] = widget.stream
        get_config()["logbook_topic"] = widget.topic
        style = self.combo_title_style.currentText()
        get_config()["logbook_header_style"] = style

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
        text = "Annotations cannot be \nremoved after cropping"
        warning_text = QLabel(text, parent=toolbar)
        font = warning_text.font()
        font.setPointSize(10)
        font.setItalic(True)
        warning_text.setFont(font)
        self._warning_action = toolbar.addWidget(warning_text)
        self._warning_action.setVisible(False)

        color_action = QAction(parent=self)
        color_action.setToolTip("Change annotation color")
        color_action.triggered.connect(self._change_color)
        toolbar.addAction(color_action)
        self.action_group_draw.addAction(color_action)
        self._color_action = color_action
        self._set_button_color()

        for tool_factory in get_tools():
            action = QAction(parent=self)
            action.setIcon(tool_factory.icon)
            action.setToolTip(tool_factory.tooltip)
            action.setCheckable(True)
            tool = tool_factory.drawing_tool()
            action.triggered.connect(partial(self.activate_tool, tool))
            toolbar.addAction(action)
            self.action_group_draw.addAction(action)

        self.action_group_draw.setExclusive(True)
        self.toolbar_layout.addWidget(toolbar)
        return toolbar

    @Slot()
    def _change_color(self):
        """Show a color dialog to choose the annotation color """
        self._warning_action.setVisible(False)
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
        warning_visible = isinstance(tool, CropTool)
        self._warning_action.setVisible(warning_visible)

    @Slot()
    def reset_tool(self):
        for action in self.action_group_draw.actions():
            action.setChecked(False)
        self.view.setCursor(Qt.ArrowCursor)
        self._warning_action.setVisible(False)
