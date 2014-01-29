#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 25, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a dialog to set the properties
   for a Text component of the middle panel. """

__all__ = ["TextDialog"]


from PyQt4.QtCore import *
from PyQt4.QtGui import *

PointSize = 10

class TextDialog(QDialog):

    def __init__(self, parent, textItem=None):
        super(TextDialog, self).__init__(parent)

        self.__teText = QTextEdit()
        self.__teText.setAcceptRichText(False)
        self.__teText.setTabChangesFocus(True)
        self.__laText = QLabel("&Text:")
        self.__laText.setBuddy(self.__teText)
        
        self.__cbFont = QFontComboBox()
        self.__cbFont.setCurrentFont(QFont("Helvetica", PointSize))
        self.__laFont = QLabel("&Font:")
        self.__laFont.setBuddy(self.__cbFont)
        
        self.__sbFontSize = QSpinBox()
        self.__sbFontSize.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.__sbFontSize.setRange(6, 280)
        self.__sbFontSize.setValue(PointSize)
        self.__laFontSize = QLabel("&Size:")
        self.__laFontSize.setBuddy(self.__sbFontSize)
        
        self.__pbTextColor = QPushButton()
        self.__pbTextColor.setMaximumSize(32,32)
        self.__laTextColor = QLabel("&Text color:")
        self.__laTextColor.setBuddy(self.__pbTextColor)
        
        self.__pbBackgroundColor = QPushButton()
        self.__pbBackgroundColor.setMaximumSize(32,32)
        self.__laBackgroundColor = QLabel("&Background color:")
        self.__laBackgroundColor.setBuddy(self.__pbBackgroundColor)
        
        self.__pbOutlineColor = QPushButton()
        self.__pbOutlineColor.setMaximumSize(32,32)
        self.__laOutlineColor = QLabel("&Outline color:")
        self.__laOutlineColor.setBuddy(self.__pbOutlineColor)
        
        self.__buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        if textItem:
            self.__teText.setPlainText(textItem.text())
            self.__cbFont.setCurrentFont(textItem.font())
            self.__sbFontSize.setValue(textItem.font().pointSize())
            
            self.__textColor = textItem.textColor()
            self.__textBackgroundColor = textItem.backgroundColor()
            self.__textOutlineColor = textItem.outlineColor()
        else:
            self.__textColor = QColor(Qt.black)
            self.__textBackgroundColor = QColor(Qt.lightGray)
            self.__textOutlineColor = QColor(Qt.darkGray)
        
        # Set colors to buttons
        self.updateColorButtons()
        
        layout = QGridLayout()
        layout.addWidget(self.__laText, 0, 0)
        layout.addWidget(self.__teText, 1, 0, 1, 6)
        layout.addWidget(self.__laFont, 2, 0)
        layout.addWidget(self.__cbFont, 2, 1, 1, 2)
        layout.addWidget(self.__laFontSize, 2, 3)
        layout.addWidget(self.__sbFontSize, 2, 4, 1, 2)
        
        hLayout = QHBoxLayout()
        hLayout.addWidget(self.__laTextColor)#, 3, 0)
        hLayout.addWidget(self.__pbTextColor)#, 3, 1)
        hLayout.addWidget(self.__laBackgroundColor)#, 3, 2)
        hLayout.addWidget(self.__pbBackgroundColor)#, 3, 3)
        hLayout.addWidget(self.__laOutlineColor)#, 3, 4)
        hLayout.addWidget(self.__pbOutlineColor)#, 3, 5)
        
        layout.addLayout(hLayout, 3, 0, 1, 6)
        layout.addWidget(self.__buttonBox, 4, 0, 1, 6)
        self.setLayout(layout)

        # Signals & Slots connections
        self.__teText.textChanged.connect(self.onUpdateUi)
        self.__cbFont.currentFontChanged.connect(self.onUpdateUi)
        self.__sbFontSize.valueChanged.connect(self.onUpdateUi)
        
        self.__pbTextColor.clicked.connect(self.onTextColorClicked)
        self.__pbBackgroundColor.clicked.connect(self.onBackgroundColorClicked)
        self.__pbOutlineColor.clicked.connect(self.onOutlineColorClicked)
        
        self.__buttonBox.accepted.connect(self.onOkClicked)
        self.__buttonBox.rejected.connect(self.reject)

        self.setWindowTitle("Edit text")
        self.onUpdateUi()


    def onUpdateUi(self):
        font = self.__cbFont.currentFont()
        font.setPointSize(self.__sbFontSize.value())
        self.__teText.document().setDefaultFont(font)
        self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(self.__teText.toPlainText() > 0)


    def updateColorButtons(self):
        self.__pbTextColor.setStyleSheet(
            "background-color : rgb({},{},{});".format(
                self.__textColor.red(), self.__textColor.green(), 
                self.__textColor.blue()))

        self.__pbBackgroundColor.setStyleSheet(
            "background-color : rgb({},{},{});".format(
                self.__textBackgroundColor.red(),
                self.__textBackgroundColor.green(),
                self.__textBackgroundColor.blue()))

        self.__pbOutlineColor.setStyleSheet(
            "background-color : rgb({},{},{});".format(
                self.__textOutlineColor.red(),
                self.__textOutlineColor.green(),
                self.__textOutlineColor.blue()))


    def text(self):
        return self.__teText.toPlainText()
    
    
    def font(self):
        font = self.__cbFont.currentFont()
        font.setPointSize(self.__sbFontSize.value())
        return font


    def textColor(self):
        return self.__textColor


    def backgroundColor(self):
        return self.__textBackgroundColor


    def outlineColor(self):
        return self.__textOutlineColor


### slots ###
    def onTextColorClicked(self):
        color = QColorDialog.getColor(self.__textColor, self)
        if color.isValid():
            self.__textColor = color
            self.updateColorButtons()
            self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)


    def onBackgroundColorClicked(self):
        color = QColorDialog.getColor(self.__textBackgroundColor, self)
        if color.isValid():
            self.__textBackgroundColor = color
            self.updateColorButtons()
            self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)


    def onOutlineColorClicked(self):
        color = QColorDialog.getColor(self.__textOutlineColor, self)
        if color.isValid():
            self.__textOutlineColor = color
            self.updateColorButtons()
            self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)


    def onOkClicked(self):
        self.accept()

