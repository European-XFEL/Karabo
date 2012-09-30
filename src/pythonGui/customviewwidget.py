#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 15, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the area in the middle where
   the user can drag and drop either items of the navigation panel or items of
   the configuration panel to get a user specific view of certain properties.
   
   This widget is embedded in the CustomViewPanel.
"""

__all__ = ["CustomViewWidget"]


from userattributecustomframe import UserAttributeCustomFrame
from userdevicecustomframe import UserDeviceCustomFrame

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class CustomViewWidget(QWidget):#QScrollArea):

    def __init__(self, parent):
        super(CustomViewWidget, self).__init__(parent)
        
        # States whether the widget is draggable
        self.__isTransformActive = True
        
        self.setBackgroundRole(QPalette.Base)
        self.setAutoFillBackground(True)
        self.setAcceptDrops(True)
        
        #self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        #self.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        
        #self.setContextMenuPolicy(Qt.DefaultContextMenu)#CustomContextMenu) 

        # Actions
        #self.__acUpdate = QAction("Update", self)
        #self.__acUpdate.triggered.connect(self.onUpdate)
        
        #self.__acRemoveAll = QAction("Remove all items", self)
        #self.__acRemoveAll.triggered.connect(self.onRemoveAll)
        # Popup Menu
        #self.__popUpMenu = QMenu(self)
        #self.__popUpMenu.addAction(self.__acUpdate)
        #self.__popUpMenu.addAction(self.__acRemoveAll)
        
#    def contextMenuEvent(self, event):
#        self.__popUpMenu.move(event.globalPos())
#        self.__popUpMenu.show()
#        QWidget.contextMenuEvent(self, event)


    def _getTransformActive(self):
        return self.__isTransformActive
    def _setTransformActive(self, active):
        self.__isTransformActive = active
    isTransformWidgetActive = property(fget=_getTransformActive, fset=_setTransformActive)


# Drag & Drop events
    def dragEnterEvent(self, event):
        
        source = event.source()
        if (source is not None) and (source is not self):
            event.setDropAction(Qt.MoveAction)
            event.accept()
        
        QWidget.dragEnterEvent(self, event)


    def dragMoveEvent(self, event):

        source = event.source()
        if source is not None and not event.mimeData().hasHtml():
            pos = self._getWidgetCenterPosition(self.mapFromGlobal(QCursor.pos()), source.width()/2, source.height()/2)
            source.move(pos)

        event.setDropAction(Qt.MoveAction)
        event.accept()
        
        QWidget.dragMoveEvent(self, event)


    def dropEvent(self, event):
        
        source = event.source()
        if source is not None:
            if event.mimeData().hasHtml():
                type = event.mimeData().html()
                data = event.mimeData().data("data")
                data = data.split(',')
                
                if len(data) < 2:
                    return
                
                # Drop from NavigationTreeView or AttributeTreeWidget?
                if type == "NavigationTreeView":
                    userCustomFrame = UserDeviceCustomFrame(key=data[0], displayName=data[1], parent=self)
                elif type == "AttributeTreeWidget":
                    key = data[0]
                    item = source.getAttributeTreeWidgetItemByKey(key)
                    navigationItemType = source.getNavigationItemType()
                    userCustomFrame = UserAttributeCustomFrame(item.classAlias, item=item, key=key, parent=self, navigationItemType=navigationItemType)
                    userCustomFrame.signalRemoveUserAttributeCustomFrame.connect(self.onRemoveUserCustomFrame)

                # show to get correct size for positioning
                userCustomFrame.show()
                pos = self._getWidgetCenterPosition(self.mapFromGlobal(QCursor.pos()), userCustomFrame.width()/2, userCustomFrame.height()/2)
                userCustomFrame.move(pos)
            else:
                pos = self._getWidgetCenterPosition(self.mapFromGlobal(QCursor.pos()), source.width()/2, source.height()/2)
                source.move(pos)

        event.setDropAction(Qt.MoveAction)
        event.accept()


    def _getWidgetCenterPosition(self, pos, centerX, centerY):
        # QPoint pos, int centerX, int centerY
        pos.setX(pos.x()-centerX)
        pos.setY(pos.y()-centerY)
        return pos


### slots ###
    def onRemoveUserCustomFrame(self, userCustomFrame):
        if userCustomFrame is None:
            return
        userCustomFrame.deleteLater()
    
    
    def onUpdate(self):
        print "CustomViewWidget.onUpdate"


    def onRemoveAll(self):
        print "CustomViewWidget.onRemoveAll"
