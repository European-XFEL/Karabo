from pathlib import Path

from qtpy import uic
from qtpy.QtCore import QObject, Qt, Signal, Slot
from qtpy.QtGui import QColor, QIcon, QPixmap
from qtpy.QtWidgets import (
    QDialog, QDialogButtonBox, QListWidgetItem, QMessageBox)

from karabogui.graph.common.api import get_allowed_colors, rgba_to_hex


class CurveOptionsDialog(QDialog):

    requestCustomOptionsRestore = Signal()

    def __init__(self, curve_options: dict | None = None,
                 parent: QObject = None):
        super().__init__(parent=parent)
        ui_file = Path(__file__).parent / "curve_options.ui"

        uic.loadUi(ui_file, self)
        self._pen_color = QColor()
        self._prepare_color_combobox()
        self._load_options(curve_options)
        self.proxy_list.setCurrentRow(0)

        self.legend_name.editingFinished.connect(self._on_legend_name_changed)
        self.color_combo_box.currentIndexChanged.connect(
            self.on_color_changed)

        self.buttonBox.button(QDialogButtonBox.Ok).setText("Apply")
        self.buttonBox.button(QDialogButtonBox.Cancel).setText("Discard")
        self.buttonBox.button(QDialogButtonBox.Reset).clicked.connect(
            self._requestReset)

    def _prepare_color_combobox(self):
        """Load the combobox with the  allowed colors for the curve"""

        # Fixed width for the icon as the text is empty.
        self.color_combo_box.setFixedWidth(50)
        for single_letter, rgba_code in get_allowed_colors():
            pixmap = QPixmap(32, 32)
            pixmap.fill(QColor(*rgba_code))
            icon = QIcon(pixmap)
            hexa_code = rgba_to_hex(rgba_code)
            self.color_combo_box.addItem(icon, "", hexa_code)

    def _load_options(self, curve_options: dict):
        """Create a list item for each curve and store their options as
        user data."""
        for key, data in curve_options.items():
            list_item = QListWidgetItem(key)
            list_item.setData(Qt.UserRole, data)
            self.proxy_list.addItem(list_item)

    @Slot(int)
    def on_proxy_list_currentRowChanged(self, row):
        """Display the curve options for the selected curve"""
        item = self.proxy_list.item(row)
        if item is None:
            self.settings_widget.setEnabled(False)
            return
        self.settings_widget.setEnabled(True)
        curve_options = item.data(Qt.UserRole)
        self.property_key.setText(curve_options["key"])
        self.legend_name.setText(curve_options["legend_name"])
        pen_color = curve_options.get("pen_color")
        index = self.color_combo_box.findData(pen_color)
        if index == -1:
            self.line_group_box.setEnabled(False)
        else:
            self._pen_color = QColor(pen_color)
            self.color_combo_box.setCurrentIndex(index)

    @Slot()
    def _on_legend_name_changed(self):
        """Save the legend name for the selected curve"""
        legend_name = self.legend_name.text()
        self._update_item_data("legend_name", legend_name)

    @Slot(int)
    def on_color_changed(self, index: int):
        """Save the pen color for the selected curve"""
        pen_color = self.color_combo_box.itemData(index)
        self._update_item_data("pen_color", pen_color)

    @Slot()
    def _requestReset(self):
        """Emit a signal to discard all the custom curve options on
        accepting the question dialog."""
        reply = QMessageBox.question(
            self, "Reset All",
            "Do you want to reset to the default options?",
            QMessageBox.Yes | QMessageBox.No)
        if reply == QMessageBox.No:
            return
        self.requestCustomOptionsRestore.emit()
        self.reject()

    def _update_item_data(self, key: str, value: str):
        """A convenient method to set the data of the current item in the
        curve option"""
        item = self.proxy_list.currentItem()
        if item is None:
            return
        data = item.data(Qt.UserRole)
        data[key] = value
        item.setData(Qt.UserRole, data)

    def get_curve_options(self) -> dict:
        """Return the plotting options for each plot"""
        curve_options = {}
        for index in range(self.proxy_list.count()):
            item = self.proxy_list.item(index)
            data = item.data(Qt.UserRole)
            curve_options[data["key"]] = data
        return curve_options
