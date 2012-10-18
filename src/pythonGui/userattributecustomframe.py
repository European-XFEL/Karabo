#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a frame for the attribute which
   gets drag and dropped into the customviewwidget.
"""

__all__ = ["UserAttributeCustomFrame"]


from enums import NavigationItemTypes
from manager import Manager
from displaywidget import DisplayWidget
from displaycomponent import DisplayComponent
from editablewidget import EditableWidget
from editableapplylatercomponent import EditableApplyLaterComponent
from editablenoapplycomponent import EditableNoApplyComponent
from vacuumwidget import VacuumWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class UserAttributeCustomFrame(QFrame):
    # signals
    signalRemoveUserAttributeCustomFrame = pyqtSignal(object) # self pointer

    def __init__(self, classAlias, **params):
        super(UserAttributeCustomFrame, self).__init__()#params.get('parent'))

        self.setAttribute(Qt.WA_NoSystemBackground, True)
        self.setAcceptDrops(True)
        
        key = params.get(QString('key'))
        if key is None:
            key = params.get('key')
        text = "<html><b>Associated key: </b>%s</html>" % key
        self.setToolTip(text)
        # Register as visible instance
        Manager().newVisibleDeviceInstance(key)
        
        self.__layout = QHBoxLayout(self)
        self.__layout.setContentsMargins(0,0,0,0)
        self.__layout.setSizeConstraint(QLayout.SetMinAndMaxSize)
        
        item = params.get(QString('item'))
        if item is None:
            item = params.get('item')
        
        # Display name
        itemText = item.text(0)
        if len(itemText) < 1:
            keys = str(item.treeWidget().instanceKey).split('.', 1)
            itemText = keys[0]
        self.__displayName = QLabel(itemText)
        self.__layout.addWidget(self.__displayName)
        
        self.__contextMenu = None
        
        navigationItemType = params.get(QString('navigationItemType'))
        if navigationItemType is None:
            navigationItemType = params.get('navigationItemType')
        
        # Display component for current value(s)
        self.__displayComponent = None
        if navigationItemType is NavigationItemTypes.DEVICE_INSTANCE:
            self.__displayComponent = DisplayComponent(classAlias, **params) #("TaurusPlot", **params)
            self.__displayComponent.onValueChanged(key, item.displayComponent.value)
            self.__layout.addWidget(self.__displayComponent.widget)

        # Editable component for editable value(s)
        self.__editableComponent = None
        if item.editableComponent is not None:
            if navigationItemType is NavigationItemTypes.DEVICE_CLASS:
                self.__editableComponent = EditableNoApplyComponent(classAlias, **params)
            elif navigationItemType is NavigationItemTypes.DEVICE_INSTANCE:
                self.__editableComponent = EditableApplyLaterComponent(classAlias, **params) #("TaurusWheelEdit", **params)
                self.__editableComponent.isEditableValueInit = False
            
            self.__editableComponent.onValueChanged(key, item.editableComponent.value)
            self.__layout.addWidget(self.__editableComponent.widget)
        
        self._setupContextMenu()


    #def mouseMoveEvent(self, event):
        # Drag is not started when customviewwidget does not allow it
        #if not self.parent().isTransformWidgetActive:
        #    return
        
    #    if event.buttons() != Qt.LeftButton:
    #        return
        
    #    mimeData = QMimeData()
    #    drag = QDrag(self)
    #    drag.setMimeData(mimeData)
    #    drag.start()
        
    #    QFrame.mouseMoveEvent(self, event)


# Drop event
    def dragEnterEvent(self, event):
        #print "UserAttributeCustomFrame.dragEnterEvent"
        
        source = event.source()
        if (source is not None) and (source is not self):
            event.setDropAction(Qt.MoveAction)
            event.accept()
        
        QWidget.dragEnterEvent(self, event)


    def dragMoveEvent(self, event):
        #print "UserAttributeCustomFrame.dragMoveEvent", self
        
        source = event.source()
        if (source is not None) and (source is not self):
            if event.mimeData().hasHtml():
                type = event.mimeData().html()
                data = event.mimeData().data("data")
                data = data.split(',')
                
                if len(data) < 2:
                    return
                
                event.setDropAction(Qt.MoveAction)
                
                if type == "AttributeTreeWidget":
                    key = data[0]
                    item = source.getAttributeTreeWidgetItemByKey(key)

                    if self.__displayComponent and (self.__displayComponent.widgetCategory != DisplayWidget.getCategoryViaAlias(self, item.classAlias)):
                        event.ignore()
                    elif self.__editableComponent and (self.__displayComponent.widgetCategory != EditableWidget.getCategoryViaAlias(self, item.classAlias)):
                        event.ignore()
                    else:
                        event.accept()
        QWidget.dragMoveEvent(self, event)


    # TODO: needs to be implemented
    def dropEvent(self, event):
        #print "UserAttributeCustomFrame.dropEvent"
        source = event.source()

        if source is not None:
            if event.mimeData().hasHtml():
                type = event.mimeData().html()
                data = event.mimeData().data("data")
                data = data.split(',')
                
                if len(data) < 2:
                    return
                
                # Drop from AttributeTreeWidget?
                if type == "AttributeTreeWidget":
                    key = data[0]
                    item = source.getAttributeTreeWidgetItemByKey(key)
                    #navigationItemType = source.getNavigationItemType()
                    if self.__displayComponent:
                        self.__displayComponent.addKeyValue(key, item.displayComponent.value)
                        text = "<html><b>Associated key(s): </b>"
                        for key in self.__displayComponent.keys:
                            text += "%s<br>" % key
                        text += "</html>"
                        self.setToolTip(text)
                        Manager().newVisibleDeviceInstance(key)

        event.setDropAction(Qt.MoveAction)
        event.accept()


    def _setupContextMenu(self):
        self.setContextMenuPolicy(Qt.DefaultContextMenu)#CustomContextMenu)
        
        # Populate context menu
        self.__contextMenu = QMenu()
        
        if self.__displayComponent:
            # Sub menu for widget type change
            self.__mChangeDisplay = QMenu("Change display widget")
            widgetAliases = DisplayWidget.getAliasesViaCategory(self, self.__displayComponent.widgetCategory)
            for i in range(len(widgetAliases)):
                acChangeDisplay = self.__mChangeDisplay.addAction(widgetAliases[i])
                acChangeDisplay.triggered.connect(self.onChangeDisplayWidget)
            self.__contextMenu.addMenu(self.__mChangeDisplay)
        
        if self.__editableComponent:
            # Sub menu for widget type change
            self.__mChangeEditable = QMenu("Change editable widget")
            widgetAliases = EditableWidget.getAliasesViaCategory(self, self.__editableComponent.widgetCategory)
            for i in range(len(widgetAliases)):
                acChangeEditable = self.__mChangeEditable.addAction(widgetAliases[i])
                acChangeEditable.triggered.connect(self.onChangeEditableWidget)
            self.__contextMenu.addMenu(self.__mChangeEditable)
        
        self.__contextMenu.addSeparator()
        
        if self.__displayComponent:
            # Sub menu for widget type change
            self.__mChangeVacuum = QMenu("Change vacuum widget")
            widgetAliases = VacuumWidget.getAliasesViaCategory(self, "State")
            for i in range(len(widgetAliases)):
                acChangeVacuum = self.__mChangeVacuum.addAction(widgetAliases[i])
                acChangeVacuum.triggered.connect(self.onChangeVacuumWidget)
            self.__contextMenu.addMenu(self.__mChangeVacuum)
        
        self.__contextMenu.addSeparator()
        
        text = "Remove widget"
        self.__acRemove = QAction(QIcon(":no"), text, self)
        self.__acRemove.setStatusTip(text)
        self.__acRemove.setToolTip(text)
        self.__acRemove.triggered.connect(self.onRemove)
        self.__contextMenu.addAction(self.__acRemove)


    def showContextMenu(self, pos):
        if self.__contextMenu is None:
            return
        
        self.__contextMenu.move(pos)
        self.__contextMenu.show()


    def _clearLayout(self, layout):
        while layout.count():
            item = layout.takeAt(0)
            if isinstance(item, QWidgetItem):
                widget = item.widget()
                if isinstance(widget, QPushButton):
                    widget.close()
                else:
                    widget.deleteLater()
            else:
                self._clearLayout(item.layout())


### slots ###
    def onChangeDisplayWidget(self):
        action = self.sender()
        # Remove old display widget
        oldWidget = self.__displayComponent.widget
        # change widget
        self.__displayComponent.changeWidget(action.text())
        index = self.__layout.indexOf(oldWidget)
        oldWidget.deleteLater()
        self.__layout.removeWidget(oldWidget)
        self.__layout.insertWidget(index, self.__displayComponent.widget)


    def onChangeEditableWidget(self):
        action = self.sender()
        # Change widget
        self.__editableComponent.changeWidget(action.text())


    def onChangeVacuumWidget(self):
        action = self.sender()
        
        # Remove old display widget
        oldWidget = self.__displayComponent.widget
        # Change widget
        self.__displayComponent.changeToVacuumWidget(action.text())
        index = self.__layout.indexOf(oldWidget)
        oldWidget.deleteLater()
        self.__layout.removeWidget(oldWidget)
        self.__layout.insertWidget(index, self.__displayComponent.widget)
        #self.adjustSize()
        

    def onRemove(self):
        # Remove items from layout
        self._clearLayout(self.__layout)
                
        if self.__displayComponent:
            for key in self.__displayComponent.keys:
                # Unregister as visible instance
                Manager().removeVisibleDeviceInstance(key)
            self.__displayComponent.destroy()
            self.__displayComponent = None
        if self.__editableComponent:
            for key in self.__editableComponent.keys:
                # Unregister as visible instance
                Manager().removeVisibleDeviceInstance(key)
            self.__editableComponent.destroy()
            self.__editableComponent = None
        
        # Inform CustomViewWidget to delete this instance
        self.signalRemoveUserAttributeCustomFrame.emit(self)


    #def onChangeWidgetType(self):
    #    action = self.sender()
    #    print "change to:", action.text()

