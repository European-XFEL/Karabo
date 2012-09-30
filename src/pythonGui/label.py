#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 13, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a label which is overwritten due
   to the implementation of the mouse double click event.
   
   It is used in the AttributeLabel class and displays a ListEdit dialog to make
   changes.
"""

__all__ = ["Label"]


from listedit import ListEdit
import userattributecustomframe

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class Label(QLabel):
    # signals
    signalEditingFinished = pyqtSignal(list)
    
    def __init__(self, **params):
        super(Label, self).__init__()
        
        self.__value = params.get(QString('value'))
        if self.__value is None:
            self.__value = params.get('value')
        if self.__value is None:
            self.__value = []
        
        self.__valueType = params.get(QString('valueType'))
        if self.__valueType is None:
            self.__valueType = params.get('valueType')
        
        self.setAcceptDrops(True)

    def _value(self):
        return self.__value
    def _setValue(self, value):
        if value is None:
            return
        
        self.__value = value
        
        listLen = len(self.__value)
        maxLen = 10
        valueAsString = ""
        for i in range(listLen):
            if maxLen < 1:
                valueAsString += ".."
                break
            
            index = self.__value[i]
            valueType = type(index)
            if valueType is float:
                index = str("%.6f" %index)
            valueAsString += str(index)
            
            if i != (listLen-1):
                valueAsString += ", "
            maxLen -= 1
        
        self.blockSignals(True)
        self.setText(QString("[%1]").arg(valueAsString))
        self.blockSignals(False)
        
    value = property(fget=_value, fset=_setValue)


    def mouseDoubleClickEvent(self, event) :
        
        listEdit = ListEdit(self.__valueType, True, self.__value)
        listEdit.setTexts("Add", "&Value", "Edit")
        
        if listEdit.exec_() == QDialog.Accepted :
            
            tmpList = str()
            values = []
            
            listCount = listEdit.getListCount()
            for i in range(listCount):
                value = listEdit.getListElementAt(i)

                if isinstance(value, QString):
                    value = str(value)
                values.append(value)
                
                # As string
                tmpList += str(value)
                if i < (listCount-1):
                    tmpList += ", "
            
            self.setText(tmpList)
            self.__value = values
            self.signalEditingFinished.emit(values)


    def dragEnterEvent(self, event):
        
        source = event.source()
        if source is not None: #and (source is not self):
            event.setDropAction(Qt.MoveAction)
            event.accept()
        
        QWidget.dragEnterEvent(self, event)


    def dragMoveEvent(self, event):

        event.setDropAction(Qt.MoveAction)
        event.accept()
        
        QWidget.dragMoveEvent(self, event)


    def dropEvent(self, event):
        source = event.source()
        if source is not None:
            if type(source) is userattributecustomframe.UserAttributeCustomFrame:
                # TODO: source can have more than only 1 key... KeWe
                return

                keys = str(source.internalKey).split('.', 1)
                instanceId = keys[0]
                attributeKey = keys[1]
                systemKey = instanceId + "/" + attributeKey
                
                values = self.__value
                if self.__valueType == 'string':
                    values.append(systemKey)
                
                tmpList = str()
                nbValues = len(values)
                for i in xrange(nbValues):
                    value = values[i]
                    # Prevent QString issue
                    values[i] = str(value)
                    # as string
                    tmpList += str(value)
                    if i < (nbValues-1):
                        tmpList += ", "

                self.setText(tmpList)
                self.signalEditingFinished.emit(values)

        event.setDropAction(Qt.MoveAction)
        event.accept()

