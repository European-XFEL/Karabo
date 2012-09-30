#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a frame for the device class/instance
   which gets drag and dropped into the customviewwidget.
"""

__all__ = ["UserDeviceCustomFrame"]


from PyQt4.QtCore import *
from PyQt4.QtGui import *

class UserDeviceCustomFrame(QFrame):

    # COLORS
    WHITE = QColor(255,255,255)
    BLACK = QColor(0,0,0)
    
    LIGHT_GREEN = QColor(216,255,216)
    DARK_GREEN = QColor(0,20,0)

    LIGHT_RED = QColor(255,216,216)
    DARK_RED = QColor(240,100,100)

    LIGHT_CYAN = QColor(216,255,255)
    DARK_CYAN = QColor(100,120,120)

    MATT_BLUE = QColor(64,128,255)

    LIGHT_PINK = QColor(255,216,255)
    DARK_PINK = QColor(20,0,20)
    
    LIGHT_YELLOW = QColor(255,255,140)
    DARK_YELLOW = QColor(240,240,0)
    
    LIGHT_ORANGE = QColor(255,236,214)
    DARK_ORANGE = QColor(255,188,112)
    
    def __init__(self, **params):
        super(UserDeviceCustomFrame, self).__init__(params.get('parent'))

        self.setFrameStyle(QFrame.Box)
        self.setLineWidth(2)
        
        displayName = params.get(QString('displayName'))
        if displayName is None:
            displayName = params.get('displayName')
        self.__displayName = str(displayName)
        
        widthSize = 50
        heightSize = 35
        fm = self.fontMetrics()
        self.resize(fm.width(self.__displayName)+4*widthSize, fm.height()+heightSize)
        
        self.__isActive = True
        self.__isVisible = True
        self._setupContextMenu()


    def _setupContextMenu(self):
        self.setContextMenuPolicy(Qt.DefaultContextMenu)
        
        # main menu
        self.__mMain = QMenu(self)
        
        text = "Remove widget"
        self.__acRemove = QAction(QIcon(":no"), text, self)
        self.__acRemove.setStatusTip(text)
        self.__acRemove.setToolTip(text)
        self.__acRemove.triggered.connect(self.onRemove)
        self.__mMain.addAction(self.__acRemove)


    def paintEvent(self, event):
        if not self.__isVisible:
            return
        
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing, True)
        
        font = painter.font()
        font.setPointSize(11)
        painter.setFont(font)
        
        offset = 10
        center = offset/2
        lineLength = self.width()/4 - 4*offset
        y = self.height()/2

        # draw input
        painter.setPen(QPen(UserDeviceCustomFrame.BLACK, 1))
        painter.setBrush(QBrush(UserDeviceCustomFrame.WHITE))
        
        painter.drawEllipse(offset, y-center, offset, offset)
        startX2 = 2*offset
        endX2 = 2*offset+lineLength
        painter.drawLine(startX2, y, endX2, y)
        
        # draw device box
        if self.__isActive is True:
            painter.setBrush(QBrush(UserDeviceCustomFrame.LIGHT_ORANGE))
            painter.setPen(QPen(UserDeviceCustomFrame.DARK_ORANGE, 1))
        else:
            painter.setBrush(QBrush(UserDeviceCustomFrame.LIGHT_CYAN))
            painter.setPen(QPen(UserDeviceCustomFrame.DARK_CYAN, 1))
        
        endY1 = self.height()-offset
        
        rect = QRect(endX2, offset, self.width()/2+4*offset, self.height()-2*offset)
        painter.drawRoundedRect(rect, offset, offset)
        painter.setPen(QPen(UserDeviceCustomFrame.BLACK, 1))
        painter.drawText(rect, Qt.AlignCenter, self.__displayName)#endX2+offset, y+center, self.__displayName)
        
        # draw output
        painter.setPen(QPen(UserDeviceCustomFrame.BLACK, 1))
        painter.setBrush(QBrush(UserDeviceCustomFrame.WHITE))
        
        endX3 = self.width()-lineLength-2*offset
        endX4 = self.width()-2*offset
        
        painter.drawLine(endX3, y, endX4, y)
        painter.drawEllipse(endX4, y-center, offset, offset)


    def mouseMoveEvent(self, event):
        # Drag is not started when customviewwidget does not allow it
        if not self.parent().isTransformWidgetActive:
            return
        
        if event.buttons() != Qt.LeftButton:
            return
        
        mimeData = QMimeData()
        drag = QDrag(self)
        drag.setMimeData(mimeData)
        drag.start()
        
        QFrame.mouseMoveEvent(self, event)


    def contextMenuEvent(self, event):
        if self.__mMain is None:
            return
        
        self.__mMain.move(event.globalPos())
        self.__mMain.show()
        QFrame.contextMenuEvent(self, event)


### slots ###
    def onRemove(self):
        self.__isVisible = False
        self.update()

