#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 12, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a dialog to change a list of
   values of a certain type.
"""

__all__ = ["ListEdit"]


from collections import OrderedDict
import numpy as np

from PyQt4.QtCore import QCoreApplication, Qt
from PyQt4.QtGui import (QDialog, QPushButton, QListWidget, QListWidgetItem,
                         QInputDialog, QMessageBox, QHBoxLayout, QVBoxLayout,
                         QFontMetrics)

from karabo.middlelayer import (VectorBool, VectorDouble, VectorFloat,
                                VectorInt8, VectorUInt8, VectorInt16,
                                VectorUInt16, VectorInt32, VectorUInt32,
                                VectorInt64, VectorUInt64)

class ListEdit(QDialog):

    def __init__(self, descriptor, duplicatesOk=True, valueList=None, parent=None):
        super(ListEdit, self).__init__(parent)

        valueList = valueList if valueList is not None else []
        self.descriptor = descriptor

        self.ask = False
        self.duplicatesOk = duplicatesOk

        self.allowedChoices = OrderedDict()
        if isinstance(self.descriptor, VectorBool):
            self.allowedChoices['0'] = 0
            self.allowedChoices['1'] = 1
            
        self.setWindowTitle("Edit list")

        self.addCaption = "Add String"
        self.addLabel = "String:"
        self.editCaption = "Edit String"
        self.editLabel = self.addLabel

        hbox = QHBoxLayout(self)
        self.__listWidget = QListWidget(self)
        self.__listWidget.currentItemChanged[
            QListWidgetItem, QListWidgetItem].connect(self.onUpdateButtons)
        hbox.addWidget(self.__listWidget)

        vbox = QVBoxLayout()
        button = QPushButton("&Add...", self)
        button.clicked.connect(self.onAddClicked)
        vbox.addWidget(button)

        self.editButton = QPushButton("&Edit...", self)
        self.editButton.clicked.connect(self.onEditClicked)
        vbox.addWidget(self.editButton)

        self.removeButton = QPushButton("&Delete", self)
        self.removeButton.clicked.connect(self.onRemoveClicked)
        vbox.addWidget(self.removeButton)

        self.upButton = QPushButton("&Up", self)
        self.upButton.clicked.connect(self.onMoveUpClicked)
        vbox.addWidget(self.upButton)

        self.downButton = QPushButton("&Down", self)
        self.downButton.clicked.connect(self.onMoveDownClicked)
        vbox.addWidget(self.downButton)
        vbox.addStretch(1)

        button = QPushButton("OK", self)
        button.clicked.connect(self.accept)
        vbox.addWidget(button)

        button = QPushButton("Cancel", self)
        button.clicked.connect(self.reject)
        vbox.addWidget(button)

        hbox.addLayout(vbox)
        self.setList(valueList)


    def setTexts(self, addCaption, addLabel, editCaption, editLabel=""):
        self.addCaption = addCaption
        self.addLabel = addLabel
        self.editCaption = editCaption
        if len(editLabel) < 1:
            self.editLabel = addLabel
        else:
            self.editLabel = editLabel


    def setList(self, valueList):
        self.__listWidget.clear()

        fm = QFontMetrics(self.__listWidget.font())
        width = 0
        for index in valueList:
            # Insert item
            if isinstance(index, np.bool_):
                index = int(index)
            self._addItem(index)

            w = fm.width(str(index))
            if w > width:
                width = w

        if self.__listWidget.verticalScrollBar() is not None:
            width += self.__listWidget.verticalScrollBar().width()

        self.__listWidget.setMinimumWidth(min(
            width,
            QCoreApplication.instance().desktop().screenGeometry().width() *
            4 / 5)) #?
        self.onUpdateButtons()


    def _addItem(self, value):
        item = QListWidgetItem(str(value))
        item.editableValue = value
        self.__listWidget.addItem(item)


    def getListCount(self):
        return self.__listWidget.count()


    def getListElementAt(self, index):
        return self.__listWidget.item(index).editableValue


    def retrieveAnyString(self, caption, label):
        currentItem = self.__listWidget.currentItem()
        if currentItem is None:
            currentValue = None
        else:
            currentValue = currentItem.editableValue

        dialog = QInputDialog.getText
        if isinstance(self.descriptor, (VectorDouble, VectorFloat)):
            dialog = QInputDialog.getDouble
        elif isinstance(self.descriptor, (VectorInt8, VectorUInt8,
                                          VectorInt16, VectorUInt16,
                                          VectorInt32, VectorUInt32,
                                          VectorInt64, VectorUInt64)):
            dialog = QInputDialog.getInt

        if currentValue is None:
            currentValue, ok = dialog(self, caption, label)
        elif dialog == QInputDialog.getText:
            currentValue, ok = dialog(self, caption, label, text=currentValue)
        elif dialog == QInputDialog.getInt:
            currentValue, ok = dialog(self, caption, label, int(currentValue))
        elif dialog == QInputDialog.getDouble:
            currentValue, ok = dialog(self, caption, label, float(currentValue))

        if ok:
            return currentValue
        else:
            return None


    def retrieveChoice(self, title, label):
        ok = False
        currentText = ""
        if self.__listWidget.currentItem() is not None:
            currentText = self.__listWidget.currentItem().text()

        keys = list(self.allowedChoices.keys())
        if currentText in keys:
            index = keys.index(currentText)
        else:
            index = 0
        text, ok = QInputDialog.getItem(self, title, label, keys, index, False)
        
        if ok:
            return self.allowedChoices[text]


    def onAddClicked(self):
        if len(self.allowedChoices) < 1:
            value = self.retrieveAnyString(self.addCaption, self.addLabel);
        else:
            value = self.retrieveChoice(self.addCaption, self.addLabel);

        if (value is None or not self.duplicatesOk and
            self.__listWidget.findItems(str(value), Qt.MatchCaseSensitive)):
            return

        self._addItem(value)
        self.__listWidget.setCurrentRow(self.__listWidget.count() - 1)
        self.onUpdateButtons()


    def onEditClicked(self):
        if len(self.allowedChoices) < 1:
            value = self.retrieveAnyString(self.editCaption, self.editLabel)
        else:
            value = self.retrieveChoice(self.editCaption, self.editLabel)

        if (value is None or not self.duplicatesOk and
            self.__listWidget.findItems(str(value), Qt.MatchCaseSensitive)):
            return
        
        currentItem = self.__listWidget.currentItem()
        currentItem.editableValue = value
        currentItem.setText(str(value))
        self.onUpdateButtons()


    def onRemoveClicked(self):
        original = self.__listWidget.currentItem().text()
        if self.ask and QMessageBox.question(
                self, "Delete", "Delete '{}'?".format(original),
                QMessageBox.Yes | QMessageBox.Default,
                QMessageBox.No | QMessageBox.Escape
            ) == QMessageBox.No:
            return
        self.__listWidget.takeItem(self.__listWidget.currentRow())
        self.onUpdateButtons()


    def onMoveUpClicked(self):
        row = self.__listWidget.currentRow()
        l = self.__listWidget
        if row > 0:
            tmpText = l.item(row - 1).text()
            l.item(row - 1).setText(l.item(row).text())
            l.item(row).setText(tmpText)
            l.item(row).editableValue, l.item(row - 1).editableValue = \
                l.item(row - 1).editableValue, l.item(row).editableValue
            l.setCurrentRow(row - 1)
            self.onUpdateButtons()



    def onMoveDownClicked(self):
        row = self.__listWidget.currentRow()
        l = self.__listWidget
        if row < l.count() - 1:
            tmpText = l.item(row + 1).text()
            l.item(row + 1).setText(l.item(row).text())
            l.item(row).setText(tmpText)
            l.item(row).editableValue, l.item(row + 1).editableValue = \
                l.item(row + 1).editableValue, l.item(row).editableValue
            l.setCurrentRow(row + 1)
            self.onUpdateButtons()


    def onUpdateButtons(self):
        hasItems = self.__listWidget.count() > 0
        self.editButton.setEnabled(hasItems)
        self.removeButton.setEnabled(hasItems)
        i = self.__listWidget.currentRow()
        self.upButton.setEnabled(hasItems and i > 0)
        self.downButton.setEnabled(hasItems and
                                   i < self.__listWidget.count() - 1)
