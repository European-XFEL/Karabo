#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 7, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a dialog to change a list of
   string values.
"""

__all__ = ["StringListEdit"]


from PyQt4.QtGui import (QDialog, QHBoxLayout, QInputDialog, QListWidget, QMessageBox, 
                         QPushButton, QVBoxLayout)


class StringListEdit(QDialog):

    def __init__(self, duplicatesOk=True, stringList=[], parent=None):
        super(StringListEdit, self).__init__(parent)

        self.ask = False
        self.duplicatesOk = duplicatesOk

        self.allowedChoices = []
        self.choiceItemList = []

        self.setWindowTitle("Edit list")

        self.addCaption = "Add String"
        self.addLabel = "String:"
        self.editCaption = "Edit String"
        self.editLabel = self.addLabel

        hbox = QHBoxLayout(self)
        self.listWidget = QListWidget(self)
        self.listWidget.currentItemChanged.connect(self.onUpdateButtons)
        hbox.addWidget(self.listWidget)

        vbox = QVBoxLayout()
        button = QPushButton("&Add...", self)
        button.clicked.connect(self.onAddString)
        vbox.addWidget(button)

        self.editButton = QPushButton("&Edit...", self)
        self.editButton.clicked.connect(self.onEditString)
        vbox.addWidget(self.editButton)

        self.removeButton = QPushButton("&Remove", self)
        self.removeButton.clicked.connect(self.onRemoveString)
        vbox.addWidget(self.removeButton)

        self.upButton = QPushButton("&Up", self)
        self.upButton.clicked.connect(self.onMoveUp)
        vbox.addWidget(self.upButton)

        self.downButton = QPushButton("&Down", self)
        self.downButton.clicked.connect(self.onMoveDown)
        vbox.addWidget(self.downButton)
        vbox.addStretch(1)

        button = QPushButton("OK", self)
        button.clicked.connect(self.onOkClicked)
        vbox.addWidget(button)

        button = QPushButton("Cancel", self)
        button.clicked.connect(self.reject)
        vbox.addWidget(button)

        hbox.addLayout(vbox)
        self.setList(stringList)


    def setTexts(self, addCaption, addLabel, editCaption, editLabel=""):
        self.addCaption = addCaption
        self.addLabel = addLabel
        self.editCaption = editCaption
        if len(editLabel) < 1:
            self.editLabel = addLabel
        else:
            self.editLabel = editLabel


    def setList(self, list):
        self.listWidget.clear()
        self.listWidget.insertItems(0, list)
        fm = QFontMetrics(self.listWidget.font())
        width = 0

        for i in range(len(list)) :
            w = fm.width(list[i])
            if w > width :
                width = w

        if self.listWidget.verticalScrollBar() is not None :
            width += self.listWidget.verticalScrollBar().width()
        self.listWidget.setMinimumWidth(min(width, QCoreApplication.instance().desktop().screenGeometry().width() * 4 / 5)) #?
        self.onUpdateButtons()


    def getList(self):
        list = []
        for i in range(self.listWidget.count()) :
            list.append(str(self.listWidget.item(i).text()))
        return list


    def getListCount(self):
        return self.listWidget.count()


    def getListElementAt(self, index):
        #if index in self.listWidget :
        return str(self.listWidget.item(index).text())


    def getListItemAt(self, index):
        return self.listWidget.item(index)


    def setAllowedChoices(self, allowedChoices, choiceItemList=[]):
        self.allowedChoices = allowedChoices
        self.choiceItemList = choiceItemList


    def retrieveAnyString(self, caption, label):
        ok = False
        currentText = ""
        if self.listWidget.currentItem() is not None :
            currentText = self.listWidget.currentItem().text()
        text, ok = QInputDialog.getText(self, caption, label, QLineEdit.Normal, currentText)
        if ok==True :
            return text
        else:
            return ""


    def retrieveChoice(self, caption, label):
        ok = False
        currentText = ""
        if self.listWidget.currentItem() is not None :
            currentText = self.listWidget.currentItem().text()

        index = 0
        for i in range(len(self.allowedChoices)) :
            if currentText == self.allowedChoices[i] :
                index = i
                break

        text, ok = QInputDialog.getItem(self, caption, label, self.allowedChoices, index, False)
        if ok==True :
            return text
        else :
            return ""


    def onAddString(self):
        text = ""
        if len(self.allowedChoices) == 0 :
            text = self.retrieveAnyString(self.addCaption, self.addLabel);
        else :
            text = self.retrieveChoice(self.addCaption, self.addLabel);

        if len(text) > 0:
            if self.duplicatesOk==False and len(self.listWidget.findItems(text, Qt.MatchCaseSensitive)) > 0 :
                return

            if len(self.allowedChoices) == 0 :
                self.listWidget.addItem(text)
            else :
                index = self.allowedChoices.indexOf(text)
                item = QListWidgetItem(text, self.listWidget)

            self.listWidget.setCurrentRow(self.listWidget.count()-1)
            # TODO: See how to do this
            #listWidget->ensureCurrentVisible();
            self.onUpdateButtons()


    def onEditString(self):
        if len(self.allowedChoices) == 0 :
            text = self.retrieveAnyString(self.editCaption, self.editLabel)
        else:
            text = self.retrieveChoice(self.editCaption, self.editLabel)
        if len(text) > 0:
            if self.duplicatesOk==False and self.listWidget.findItems(text, Qt.MatchCaseSensitive)!=[] :
                return
            self.listWidget.currentItem().setText(text)
            self.onUpdateButtons()


    def onRemoveString(self):
        original = self.listWidget.currentItem().text()
        if (len(original) < 1) or (self.ask and QMessageBox.question(self, "Remove",
                                                                    "Remove '{}'?".format(original),
                                                                    QMessageBox.Yes | QMessageBox.Default,
                                                                    QMessageBox.No | QMessageBox.Escape) ==
                                                                    QMessageBox.No):
            return
        self.listWidget.takeItem(self.listWidget.currentRow())
        self.onUpdateButtons()


    def onMoveUp(self):
        i = self.listWidget.currentRow()
        if i > 0 :
            temp = self.listWidget.item(i-1).text()
            self.listWidget.item(i-1).setText(self.listWidget.item(i).text())
            self.listWidget.item(i).setText(temp)
            self.listWidget.setCurrentRow(i-1)
            self.onUpdateButtons()


    def onMoveDown(self):
        i = self.listWidget.currentRow()
        if i < self.listWidget.count()-1 :
            temp = self.listWidget.item(i+1).text()
            self.listWidget.item(i+1).setText(self.listWidget.item(i).text())
            self.listWidget.item(i).setText(temp)
            self.listWidget.setCurrentRow(i+1)
            self.onUpdateButtons()


    def onUpdateButtons(self):
        hasItems = self.listWidget.count()>0
        self.editButton.setEnabled(hasItems)
        self.removeButton.setEnabled(hasItems)
        i = self.listWidget.currentRow()
        self.upButton.setEnabled(hasItems and i>0)
        self.downButton.setEnabled(hasItems and i<self.listWidget.count()-1)


    def onOkClicked(self):
        self.accept()

