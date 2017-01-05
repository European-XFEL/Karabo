#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import pyqtSignal, pyqtSlot
from PyQt4.QtGui import QDialog, QPushButton

from karabo_gui.widget import EditableWidget
from karabo.middlelayer import Hash, VectorString
from karabo_gui.stringlistedit import StringListEdit


class EditableListElement(EditableWidget):
    category = VectorString
    alias = "List Element Field"
    signalValueChanged = pyqtSignal(str, object)  # key, value

    def __init__(self, box, parent):
        super(EditableListElement, self).__init__(box)

        self._pushButton = QPushButton("Edit list", parent)
        self._pushButton.setStyleSheet("QPushButton { text-align: left; }")

        self._choice_item_list = []  # list with hidden possible items of listelement
        self._choice_string_list = []  # list with names of possible listelements
        self._selected_item_list = []  # list with already added items
        self._selected_string_list = []  # list with selected listelements

        self._is_init = False

        self._pushButton.clicked.connect(self.onClicked)

    @property
    def widget(self):
        return self._pushButton

    @property
    def value(self):
        return self._selected_string_list  # TODO: Hash(value) compare with EditableChoiceElement

    def copy_list_item(self, values, arrayIndex=0):
        if isinstance(values, list):
            for v in values:
                self._add_list_item(v, arrayIndex)
        else:
            self._add_list_item(values, arrayIndex)

    def _add_list_item(self, value, arrayIndex):
        if not self._choice_string_list:
            return
        index = self._choice_string_list.index(value)
        if index < 0:
            return

        choice_item = self._choice_item_list[index]
        parent_item = choice_item.parent()

        # Change full key name...
        new_internal_keyname = parent_item.box
        new_internal_keyname.append("[{}]".format(arrayIndex))  # [next]

        copy_item = choice_item.copy(parent_item, new_internal_keyname)
        parent_item.setExpanded(True)
        self._selected_item_list.append(copy_item)

        # Notify Manager about changes
        self.signalValueChanged.emit(copy_item.box, Hash())

    def valueChanged(self, box, value, timestamp=None):

        self._selected_string_list = value

        if self._is_init is False:
            # Copy item
            self.copy_list_item(value)
            self._is_init = True

    @pyqtSlot()
    def onClicked(self):
        listEdit = StringListEdit(True, self.value)
        listEdit.setTexts("Add", "&Name", "Edit")
        listEdit.set_allowed_choices(self._choice_string_list,
                                     self._choice_item_list)

        if listEdit.exec_() == QDialog.Accepted:
            # Remove old items
            for i in range(len(self._selected_item_list)):
                item = self._selected_item_list[i]
                parent_item = item.parent()
                if parent_item is not None:
                    parent_item.removeChild(item)
            self._selected_string_list = []

            for i in range(listEdit.get_list_count()):
                value = listEdit.get_list_element_at(i)
                self._selected_string_list.append(value)

                # TODO: don't copy already existing item..
                self.copy_list_item(listEdit.get_list_element_at(i), i)
