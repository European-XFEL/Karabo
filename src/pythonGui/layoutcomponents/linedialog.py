#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 25, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a dialog to set the properties
   for a Line component of the middle panel."""

__all__ = ["LineDialog", "PenStyleComboBox"]


from PyQt4.QtCore import *
from PyQt4.QtGui import *


class LineDialog(QDialog):

    def __init__(self, parent, lineItem=None):
        super(LineDialog, self).__init__(parent)
        
        self.__sbLineLength = QDoubleSpinBox()
        self.__sbLineLength.setRange(0.25, 999999999.0)
        self.__sbLineLength.setDecimals(2)
        self.__sbLineLength.setSingleStep(0.25)
        
        self.__sbLineWidth = QDoubleSpinBox()
        self.__sbLineWidth.setRange(0.25, 999999999.0)
        self.__sbLineWidth.setDecimals(2)
        self.__sbLineWidth.setSingleStep(0.25)
        
        self.__pbLineColor = QPushButton()
        self.__pbLineColor.setMaximumSize(32,32)
        
        self.__cbDashType = PenStyleComboBox()
        
        if lineItem:
            self.__sbLineLength.setValue(lineItem.length())
            self.__sbLineWidth.setValue(lineItem.widthF())
            self.__lineColor = lineItem.color()
            self.__cbDashType.setPenStyle(lineItem.style())
        
        self.updateLineColorButton()
        
        formLayout = QFormLayout()
        formLayout.addRow("&Length:", self.__sbLineLength)
        formLayout.addRow("&Width:", self.__sbLineWidth)
        formLayout.addRow("&Line color:", self.__pbLineColor)
        formLayout.addRow("&Dash type:", self.__cbDashType)
        
        self.__sbLineLength.valueChanged.connect(self.onLineLengthChanged)
        self.__sbLineWidth.valueChanged.connect(self.onLineWidthChanged)
        self.__pbLineColor.clicked.connect(self.onLineColorClicked)
        self.__cbDashType.currentIndexChanged.connect(self.onDashTypeChanged)
        
        self.__buttonBox = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        layout = QVBoxLayout()
        layout.addLayout(formLayout)
        layout.addWidget(self.__buttonBox)#, 0, 0, 1, 6)
        self.setLayout(layout)
        
        self.__buttonBox.accepted.connect(self.onOkClicked)
        self.__buttonBox.rejected.connect(self.reject)

        self.setWindowTitle("Edit line")


    def updateLineColorButton(self):
        self.__pbLineColor.setStyleSheet(QString("background-color : rgb(%1,%2,%3);").arg(self.__lineColor.red()) \
                                                                                     .arg(self.__lineColor.green()) \
                                                                                     .arg(self.__lineColor.blue()))


    def lineLength(self):
        return self.__sbLineLength.value()


    def lineWidthF(self):
        return self.__sbLineWidth.value()


    def lineColor(self):
        return self.__lineColor


    def penStyle(self):
        return self.__cbDashType.penStyle()


### slots ###
    def onLineLengthChanged(self, value):
        self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)


    def onLineWidthChanged(self, value):
        self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)


    def onLineColorClicked(self):
        color = QColorDialog.getColor(self.__lineColor, self)
        if color.isValid():
            self.__lineColor = color
            self.updateLineColorButton()
            self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)


    def onDashTypeChanged(self):
        self.__buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)


    def onOkClicked(self):
        self.accept()



class PenStyleComboBox(QComboBox):

    def __init__(self, parent=None):
        super(PenStyleComboBox, self).__init__(parent)

        styles = [(Qt.SolidLine, "Solid line"),
                  (Qt.DashLine, "Dashed line"),
                  (Qt.DotLine, "Dot line"),
                  (Qt.DashDotLine, "Dash dot line"),
                  (Qt.DashDotDotLine, "Dash dot dot line")]

        self.setIconSize(QSize(32, 12))
        for s in styles:
            style = s[0]
            name = s[1]
            self.addItem(self.iconForPen(style), name, QVariant(style))


    def penStyle(self):
        styleTuple = self.itemData(self.currentIndex()).toInt()
        if styleTuple[1]:
            return styleTuple[0]
        return Qt.SolidLine


    def setPenStyle(self, style):
        id = self.findData(QVariant(style))
        if id == -1:
            id = 0
        self.setCurrentIndex(id)


    def iconForPen(self, style):
        pix = QPixmap(self.iconSize())
        p = QPainter()
        pix.fill(Qt.transparent)

        p.begin(pix)
        pen = QPen(style)
        pen.setWidth(2)
        p.setPen(pen)
        
        mid = self.iconSize().height() / 2.0
        p.drawLine(0, mid, self.iconSize().width(), mid)
        p.end()

        return QIcon(pix)

