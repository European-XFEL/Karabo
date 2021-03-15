#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 6, 2021
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import numpy as np
from PyQt5.QtCore import pyqtSlot, Qt
from PyQt5.QtWidgets import (
    QComboBox, QItemDelegate)

from karabo.common.api import KARABO_SCHEMA_OPTIONS


def _get_options(binding):
    """Extract a single bindings's options

    If options are not specified, `None` is returned!
    """
    return binding.attributes.get(KARABO_SCHEMA_OPTIONS, None)


def get_table_delegate(binding, parent):
    options = _get_options(binding)
    if options is not None:
        return ComboBoxDelegate(options, parent)
    return None


class ComboBoxDelegate(QItemDelegate):
    def __init__(self, options, parent=None):
        super(ComboBoxDelegate, self).__init__(parent)
        # XXX: We might have options from number values, they serialize as
        # ndarray! Once the value casting is back, the string cast has
        # to be removed!
        if isinstance(options, np.ndarray):
            options = options.astype(np.str).tolist()
        self._options = options

    def createEditor(self, parent, option, index):
        """Reimplemented function of QItemDelegate"""
        combo = QComboBox(parent)
        combo.addItems([str(o) for o in self._options])
        combo.currentIndexChanged.connect(self._on_editor_changed)
        return combo

    def setEditorData(self, editor, index):
        """Reimplemented function of QItemDelegate"""
        selection = index.model().data(index, Qt.DisplayRole)
        try:
            selection_index = self._options.index(selection)
            editor.blockSignals(True)
            editor.setCurrentIndex(selection_index)
            editor.blockSignals(False)
        except ValueError:
            # XXX: Due to schema injection, a property value might not be in
            # allowed options and thus not available. This is very rare
            # and unlikely and we just continue!
            raise RuntimeError("The value {} is not in the following "
                               "options: {}".format(selection, self._options))

    def setModelData(self, editor, model, index):
        """Reimplemented function of QItemDelegate"""
        model.setData(index, self._options[editor.currentIndex()], Qt.EditRole)

    @pyqtSlot()
    def _on_editor_changed(self):
        """The current index of the combobox changed. Notify the model.

        This signal MUST be emitted when the editor widget has completed
        editing the data, and wants to write it back into the model.
        """
        self.commitData.emit(self.sender())
