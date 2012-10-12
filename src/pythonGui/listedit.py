#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 12, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a dialog to change a list of
   values of a certain type.
"""

__all__ = ["ListEdit"]


import const

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class ListEdit(QDialog):

    def __init__(self, valueType, duplicatesOk=True, list=[], parent=None):
        super(ListEdit, self).__init__(parent)

        self.__valueType = valueType

        self.ask = False
        self.duplicatesOk = duplicatesOk

        self.parentItem = None
        self.allowedChoices = []
        self.choiceItemList = []

        self.setWindowTitle("Edit list")

        self.addCaption = "Add String"
        self.addLabel = "String:"
        self.editCaption = "Edit String"
        self.editLabel = self.addLabel

        hbox = QHBoxLayout(self)
        self.__listWidget = QListWidget(self)
        self.connect(self.__listWidget, SIGNAL('currentItemChanged(QListWidgetItem*, QListWidgetItem*)'), self.onUpdateButtons)
        hbox.addWidget(self.__listWidget)

        vbox = QVBoxLayout()
        button = QPushButton("&Add...", self)
        button.clicked.connect(self.onAddClicked)
        vbox.addWidget(button)

        self.editButton = QPushButton("&Edit...", self)
        self.editButton.clicked.connect(self.onEditClicked)
        vbox.addWidget(self.editButton)

        self.removeButton = QPushButton("&Remove", self)
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
        button.clicked.connect(self.onOkClicked)
        vbox.addWidget(button)

        button = QPushButton("Cancel", self)
        button.clicked.connect(self.reject)
        vbox.addWidget(button)

        hbox.addLayout(vbox)
        self.setList(list)


    def setTexts(self, addCaption, addLabel, editCaption, editLabel=QString()):
        self.addCaption = addCaption
        self.addLabel = addLabel
        self.editCaption = editCaption
        if editLabel.isEmpty() == True :
            self.editLabel = addLabel
        else:
            self.editLabel = editLabel


    def setList(self, list):
        self.__listWidget.clear()
        
        fm = QFontMetrics(self.__listWidget.font())
        width = 0
        for index in list:
            # insert item
            self._addItem(index)
            
            w = fm.width(QString(index))
            if w > width :
                width = w

        if self.__listWidget.verticalScrollBar() is not None :
            width += self.__listWidget.verticalScrollBar().width()
        
        self.__listWidget.setMinimumWidth(min(width, QCoreApplication.instance().desktop().screenGeometry().width() * 4 / 5)) #?
        self.onUpdateButtons()


    def _addItem(self, value):
        item = QListWidgetItem(str(value))
        if isinstance(value, QString):
            value = str(value)
        item.setData(const.CURRENT_EDITABLE_VALUE, value)
        self.__listWidget.addItem(item)


    def getListCount(self):
        return self.__listWidget.count()


    def getListElementAt(self, index):
        return self.__listWidget.item(index).data(const.CURRENT_EDITABLE_VALUE).toPyObject()


    def setAllowedChoices(self, allowedChoices, parentItem=None, choiceItemList=[]):
        self.parentItem = parentItem
        self.allowedChoices = allowedChoices
        self.choiceItemList = choiceItemList


    def retrieveAnyString(self, caption, label):

        currentItem = self.__listWidget.currentItem()
        if currentItem is None:
            currentValue = None
        else:
            currentValue = currentItem.data(const.CURRENT_EDITABLE_VALUE).toPyObject()

        if self.__valueType == "float":
            if currentValue is None:
                currentValue, ok = QInputDialog.getDouble(self, caption, label)
            else:
                currentValue, ok = QInputDialog.getDouble(self, caption, label, currentValue)
        elif self.__valueType == "int":
            if currentValue is None:
                currentValue, ok = QInputDialog.getInt(self, caption, label)
            else:
                currentValue, ok = QInputDialog.getInt(self, caption, label, currentValue)
        else:
            if currentValue is None:
                currentValue, ok = QInputDialog.getText(self, caption, label, QLineEdit.Normal)
            else:
                currentValue, ok = QInputDialog.getText(self, caption, label, QLineEdit.Normal, currentValue)
            currentValue = str(currentValue)
        
        if ok is True :
            return currentValue
        else:
            return str()


    def retrieveChoice(self, caption, label):
        # TODO
        
        ok = False
        currentText = str()
        if self.__listWidget.currentItem() is not None :
            currentText = str(self.__listWidget.currentItem().text())

        index = 0
        for i in range(len(self.allowedChoices)) :
            if currentText == self.allowedChoices[i] :
                index = i
                break

        text, ok = QInputDialog.getItem(self, caption, label, self.allowedChoices, index, False)
        if ok==True :
            return text
        else :
            return QString()


### slots ###
    def onAddClicked(self):

        if len(self.allowedChoices) < 1 :
            value = self.retrieveAnyString(self.addCaption, self.addLabel);
        else :
            value = self.retrieveChoice(self.addCaption, self.addLabel);

        valueAsString = str(value)
        if len(valueAsString) < 1:
            return
        if self.duplicatesOk==False and self.__listWidget.findItems(valueAsString, Qt.MatchCaseSensitive)!=[] :
            return

        self._addItem(value)
        self.__listWidget.setCurrentRow(self.__listWidget.count()-1)
        self.onUpdateButtons()


    def onEditClicked(self):
        
        if len(self.allowedChoices) < 1 :
            value = self.retrieveAnyString(self.editCaption, self.editLabel)
        else:
            value = self.retrieveChoice(self.editCaption, self.editLabel)
        
        valueAsString = str(value)
        if len(valueAsString) < 1:
            return
        if self.duplicatesOk==False and self.__listWidget.findItems(valueAsString, Qt.MatchCaseSensitive)!=[] :
            return
        currentItem = self.__listWidget.currentItem()
        currentItem.setData(const.CURRENT_EDITABLE_VALUE, value)
        currentItem.setText(valueAsString)
        self.onUpdateButtons()


    def onRemoveClicked(self):
        original = self.__listWidget.currentItem().text()
        if original.isEmpty()==True or (self.ask and QMessageBox.question(self, "Remove",
                                                                                QString("Remove '%1'?").arg(original),
                                                                                QMessageBox.Yes | QMessageBox.Default,
                                                                                QMessageBox.No | QMessageBox.Escape) ==
                                                                                QMessageBox.No) :
            return
        self.__listWidget.takeItem(self.__listWidget.currentRow())
        self.onUpdateButtons()


    def onMoveUpClicked(self):
        row = self.__listWidget.currentRow()
        if row > 0 :
            tmpText = self.__listWidget.item(row-1).text()
            tmpData = self.__listWidget.item(row-1).data(const.CURRENT_EDITABLE_VALUE)
            
            self.__listWidget.item(row-1).setText(self.__listWidget.item(row).text())
            self.__listWidget.item(row-1).setData(const.CURRENT_EDITABLE_VALUE, self.__listWidget.item(row).data(const.CURRENT_EDITABLE_VALUE))
            
            self.__listWidget.item(row).setText(tmpText)
            self.__listWidget.item(row).setData(const.CURRENT_EDITABLE_VALUE, tmpData)
            
            self.__listWidget.setCurrentRow(row-1)
            self.onUpdateButtons()



    def onMoveDownClicked(self):
        row = self.__listWidget.currentRow()
        if row < self.__listWidget.count()-1 :
            tmpText = self.__listWidget.item(row+1).text()
            tmpData = self.__listWidget.item(row+1).data(const.CURRENT_EDITABLE_VALUE)
            
            self.__listWidget.item(row+1).setText(self.__listWidget.item(row).text())
            self.__listWidget.item(row+1).setData(const.CURRENT_EDITABLE_VALUE, self.__listWidget.item(row).data(const.CURRENT_EDITABLE_VALUE))
            
            self.__listWidget.item(row).setText(tmpText)
            self.__listWidget.item(row).setData(const.CURRENT_EDITABLE_VALUE, tmpData)
            
            self.__listWidget.setCurrentRow(row+1)
            self.onUpdateButtons()


    def onUpdateButtons(self):
        hasItems = self.__listWidget.count()>0
        self.editButton.setEnabled(hasItems)
        self.removeButton.setEnabled(hasItems)
        i = self.__listWidget.currentRow()
        self.upButton.setEnabled(hasItems and i>0)
        self.downButton.setEnabled(hasItems and i<self.__listWidget.count()-1)


    def onOkClicked(self):
        self.accept()

