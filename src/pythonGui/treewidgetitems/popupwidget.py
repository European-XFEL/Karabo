#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 21, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a popup dialog which replaces
   the tooltip of a treewidgetitem.
"""

__all__ = ["PopupWidget"]


from PyQt4.QtCore import *
from PyQt4.QtGui import *


class PopupWidget(QWidget):
    
    def __init__(self, parent=None):
        super(PopupWidget, self).__init__(parent, Qt.Drawer)
        
        self.__teInfo = TextEdit(self)
        self.__teInfo.setReadOnly(True)
        
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0,0,0,0)
        layout.addWidget(self.__teInfo)
        
        self.setWindowTitle(QString(" "))


    def setInfo(self, info):
        
        htmlString = QString("<table>")
        for pair in info.items():
            htmlString.append("<tr><td><b>" + str(pair[0]) + "</b>:   </td>" + "<td>" + str(pair[1]) + "</td></tr>")
        htmlString.append("</table")
        self.__teInfo.setHtml(htmlString)
        
        self.__teInfo.fitHeightToContent(len(info))


class TextEdit(QTextEdit):
    
    def __init__(self, parent=None):
        super(TextEdit, self).__init__(parent)

        self.__fittedHeight = QSize()


    def sizeHint(self):
        sizeHint = QTextEdit.sizeHint(self)
        sizeHint.setHeight(self.__fittedHeight)
        return sizeHint


    def fitHeightToContent(self, nbInfoKeys):
        self.__fittedHeight = self.fontMetrics().height() * nbInfoKeys
        self.updateGeometry()

