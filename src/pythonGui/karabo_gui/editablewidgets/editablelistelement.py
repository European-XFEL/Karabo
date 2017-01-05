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

        self.__pushButton = QPushButton("Edit list", parent)
        self.__pushButton.setStyleSheet("QPushButton { text-align: left; }")

        self.__choiceItemList = []  # list with hidden possible items of listelement
        self.__choiceStringList = []  # list with names of possible listelements
        self.__selectedItemList = []  # list with already added items
        self.__selectedStringList = []  # list with selected listelements

        self.__isInit = False

        self.__pushButton.clicked.connect(self.onClicked)

    @property
    def widget(self):
        return self.__pushButton

    @property
    def value(self):
        return self.__selectedStringList  # TODO: Hash(value) compare with EditableChoiceElement

    def copy_list_item(self, values, arrayIndex=0):
        if isinstance(values, list):
            for v in values:
                self._addListItem(v, arrayIndex)
        else:
            self._addListItem(values, arrayIndex)

    def _addListItem(self, value, arrayIndex):
        if not self.__choiceStringList:
            return
        index = self.__choiceStringList.index(value)
        if index < 0:
            return

        choiceItem = self.__choiceItemList[index]
        parentItem = choiceItem.parent()

        # Change full key name...
        newInternalKeyName = parentItem.box
        newInternalKeyName.append("[{}]".format(arrayIndex))  # [next]

        copyItem = choiceItem.copy(parentItem, newInternalKeyName)
        parentItem.setExpanded(True)
        self.__selectedItemList.append(copyItem)

        # Notify Manager about changes
        self.signalValueChanged.emit(copyItem.box, Hash())

    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            return

        self.__selectedStringList = value

        if self.__isInit is False:
            # Copy item
            self.copy_list_item(value)
            self.__isInit = True

    @pyqtSlot()
    def onClicked(self):
        listEdit = StringListEdit(True, self.value)
        listEdit.setTexts("Add", "&Name", "Edit")
        listEdit.set_allowed_choices(self.__choiceStringList,
                                   self.__choiceItemList)

        if listEdit.exec_() == QDialog.Accepted:
            # Remove old items
            for i in range(len(self.__selectedItemList)):
                item = self.__selectedItemList[i]
                parentItem = item.parent()
                if parentItem is not None:
                    parentItem.removeChild(item)
            self.__selectedStringList = []

            for i in range(listEdit.getListCount()):
                value = listEdit.get_list_element_at(i)
                self.__selectedStringList.append(value)

                # TODO: don't copy already existing item..
                self.copy_list_item(listEdit.get_list_element_at(i), i)
