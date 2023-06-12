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
from qtpy.QtCore import QPoint, QSize, Qt, Slot
from qtpy.QtWidgets import QDialog, QDialogButtonBox, QTableWidgetItem

from .utils import get_dialog_ui


class ReplaceDialog(QDialog):
    def __init__(self, devices, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui('replacedialog.ui'), self)

        self.twTable.setRowCount(len(devices))
        for i, d in enumerate(devices):
            item = QTableWidgetItem(d)
            item.setFlags(item.flags() & ~Qt.ItemIsEditable)
            self.twTable.setItem(i, 0, item)

            item = QTableWidgetItem(d)
            self.twTable.setItem(i, 1, item)
            self.twTable.editItem(item)
        self.twTable.itemChanged.connect(self.onItemChanged)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

    def mappedDevices(self):
        """
        A dict with the mapped devices is returned.
        """
        retval = {}
        for i in range(self.twTable.rowCount()):
            item_text = self.twTable.item(i, 1).text()
            retval[self.twTable.item(i, 0).text()] = item_text
        return retval

    @Slot(QTableWidgetItem)
    def onItemChanged(self, item):
        if item.column() != 1:
            return
        text = item.text()
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(len(text) > 0)


class SceneItemDialog(QDialog):
    """A dialog to modify the layout bounding rect coordinates
    """

    def __init__(self, x=0, y=0, title='SceneItem', max_x=1024, max_y=768,
                 parent=None):
        super().__init__(parent)
        filepath = get_dialog_ui('sceneitem_dialog.ui')
        uic.loadUi(filepath, self)
        self.setModal(False)
        # Fill the dialog with start values!
        self.ui_x.setValue(x)
        self.ui_x.setMaximum(max_x)
        self.ui_y.setValue(y)
        self.ui_y.setMaximum(max_y)
        self.setWindowTitle(title)
        if parent is not None:
            # place dialog accordingly!
            point = parent.rect().bottomRight()
            global_point = parent.mapToGlobal(point)
            self.move(global_point - QPoint(self.width(), 0))

    @property
    def x(self):
        return self.ui_x.value()

    @property
    def y(self):
        return self.ui_y.value()


class ResizeSceneDialog(QDialog):
    def __init__(self, size=QSize(), parent=None):
        super().__init__(parent)
        self.setModal(False)
        ui_path = get_dialog_ui('resizedialog.ui')
        uic.loadUi(ui_path, self)
        self.width_spinbox.setValue(size.width())
        self.height_spinbox.setValue(size.height())
        self.button_box.accepted.connect(self.accept)
        self.button_box.rejected.connect(self.reject)

    @property
    def scene_size(self):
        return QSize(self.width_spinbox.value(), self.height_spinbox.value())
