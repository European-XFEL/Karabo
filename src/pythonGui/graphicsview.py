#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 21, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a QGraphicsView."""

__all__ = ["GraphicsView"]

from customxmlreader import CustomXmlReader
from customxmlwriter import CustomXmlWriter

from displaycomponent import DisplayComponent

from enums import NavigationItemTypes
from graphicsscene import GraphicsScene

from layoutcomponents.arrow import Arrow
from editableapplylatercomponent import EditableApplyLaterComponent
from editablenoapplycomponent import EditableNoApplyComponent
from layoutcomponents.graphicscustomitem import GraphicsCustomItem
from layoutcomponents.graphicsproxywidget import GraphicsProxyWidget
from layoutcomponents.graphicsproxywidgetcontainer import GraphicsProxyWidgetContainer
from layoutcomponents.line import Line
from layoutcomponents.link import Link
from layoutcomponents.linkbase import LinkBase
from layoutcomponents.nodebase import NodeBase
from layoutcomponents.rectangle import Rectangle
from layoutcomponents.text import Text
from layoutcomponents.textdialog import TextDialog

#from userattributecustomframe import UserAttributeCustomFrame
#from userdevicecustomframe import UserDeviceCustomFrame

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class GraphicsView(QGraphicsView):
    # Enums
    MoveItem, InsertText, InsertLine, InsertRect = range(4)
    # Signals
    lineInserted = pyqtSignal()
    rectInserted = pyqtSignal()
    sceneSelectionChanged = pyqtSignal()

    def __init__(self):
        super(GraphicsView, self).__init__()
        
        self.__scene = GraphicsScene(0, 0, 600, 500)
        self.__scene.selectionChanged.connect(self.onSceneSelectionChanged)
        self.setScene(self.__scene)

        # Current mode of the view (move, insert
        self.__mode = self.MoveItem
        
        self.__line = None
        self.__rect = None
        
        self.setDesignMode(True)
        
        self.__minZ = 0
        self.__maxZ = 0
        self.__seqNumber = 0
        
        self.__rotAngle = 30
        
        # Describes most recent item to be cut or copied inside the application
        self.__copiedItem = QByteArray()
        
        self.setAcceptDrops(True)
        self.setDragMode(QGraphicsView.RubberBandDrag)
        self.setRenderHints(QPainter.Antialiasing | QPainter.TextAntialiasing)


    def _getMode(self):
        return self.__mode
    def _setMode(self, mode):
        self.__mode = mode
    mode = property(fget=_getMode, fset=_setMode)


    def _getDesignMode(self):
        return self.__isDesignMode
    isDesignMode = property(fget=_getDesignMode)


    # Sets all items in design or control mode
    def setDesignMode(self, isDesignMode):
        self.__isDesignMode = isDesignMode
        if self.__isDesignMode:
            self.scene().setBackgroundBrush(QBrush(QPixmap(':grid-edit')))
        else:
            self.scene().setBackgroundBrush(QBrush())
        for item in self.items():
            if isinstance(item, NodeBase):
                item.isDesignMode = isDesignMode


    # Returns true, when items has been copied; otherwise false
    def hasCopy(self):
        return not self.__copiedItem.isEmpty()


    # All selected items of the scene are returned
    def selectedItems(self):
        return self.__scene.selectedItems()


    # If there are exactely 2 selected items (not of type Link) they are returned
    # as a pair; otherwise None is returned
    def selectedItemPair(self):
        items = self.selectedItems()
        if len(items) == 2:
            firstItem = items[0]
            secondItem = items[1]
            
            if isinstance(firstItem, Link):
                firstItem = None
            if isinstance(secondItem, Link):
                secondItem = None
            
            if firstItem and secondItem:
                return (firstItem, secondItem)
        
        return None


    # All selected items of type Text are returned
    def selectedTextItems(self):
        items = self.selectedItems()
        
        if len(items) > 0:
            textItems = []
            for item in items:
                if isinstance(item, Text):
                    textItems.append(item)
            return textItems
        return None
        

    # All selected items of type Link are returned
    def selectedLinks(self):
        items = self.selectedItems()
        
        if len(items) > 0:
            linkItems = []
            for item in items:
                if isinstance(item, LinkBase):
                    linkItems.append(item)
            if len(linkItems) > 0:
                return linkItems
        return None


    def selectedItemGroup(self):
        items = self.selectedItems()
        
        if len(items) > 0:
            groupItems = []
            for item in items:
                if isinstance(item, QGraphicsItemGroup):
                    groupItems.append(item)
            if len(groupItems) > 0:
                return groupItems
        return None


    # Open saved view from file
    def openSceneFromFile(self):
        filename = QFileDialog.getOpenFileName(None, "Open saved view", QDir.tempPath(), "SCENE (*.scene)")
        if filename.isEmpty():
            return
        
        file = QFile(filename)
        if file.open(QIODevice.ReadOnly | QIODevice.Text) == False:
            return
        
        xmlContent = str()
        while file.atEnd() == False:
            xmlContent += str(file.readLine())
        
        self.removeItems(self.items())
        CustomXmlReader(self).read(xmlContent)


    # Save active view to file
    def saveSceneToFile(self):
        filename = QFileDialog.getSaveFileName(None, "Save file as", QDir.tempPath(), "SCENE (*.scene)")
        if filename.isEmpty():
            return
        
        fi = QFileInfo(filename)
        if fi.suffix().isEmpty():
            filename += ".scene"
        
        CustomXmlWriter(self.__scene).write(filename)


    # A new instance of a text is created and passed to the _setupItem function
    # to position and select
    def addText(self):
        textDialog = TextDialog(self)
        if textDialog.exec_() == QDialog.Rejected:
            return
        
        textItem = Text(self.__isDesignMode)
        textItem.setText(textDialog.text())
        textItem.setFont(textDialog.font())
        textItem.setTextColor(textDialog.textColor())
        textItem.setBackgroundColor(textDialog.backgroundColor())
        textItem.setOutlineColor(textDialog.outlineColor())
        
        #node.setText(QString("Node %1").arg(self.__seqNumber + 1))
        self._setupItem(textItem)


    # Add a link, if exactely 2 items are selected
    def addLink(self):
        items = self.selectedItemPair()
        if items is None:
            return
        
        link = Link(items[0], items[1])
        self.__scene.addItem(link)

    
    # Add an arrow, if exactely 2 items are selected
    def addArrowLink(self):
        items = self.selectedItemPair()
        if items is None:
            return
        
        arrowLink = Arrow(items[0], items[1])
        self.__scene.addItem(arrowLink)


    # Cut is two-part process: copy selected items and remove item from scene
    def cut(self):
        items = self.selectedItems()
        if len(items) < 1:
            return
        
        # Copy items
        self.copy()
        # Remove items from scene
        for item in items:
            self.__scene.removeItem(item)
            del item


    # The selected items are stored as binary data inside the application
    def copy(self):
        items = self.selectedItems()
        if len(items) < 1:
            return

        # Copy data into DataStream
        self.__copiedItem.clear()
        stream = QDataStream(self.__copiedItem, QIODevice.WriteOnly)
        for item in items:
            if isinstance(item, Text):
                stream << QString("Text") << item.text() \
                                          << item.font().toString() \
                                          << item.textColor().name() \
                                          << item.outlineColor().name() \
                                          << item.backgroundColor().name() \
                                          << QString("\n")
            elif isinstance(item, Link):
                print "Link"
            elif isinstance(item, Arrow):
                print "Arrow"
            elif isinstance(item, Line):
                line = item.line()
                stream << QString("Line") << QString("%1,%2,%3,%4").arg(line.x1()).arg(line.y1()).arg(line.x2()).arg(line.y2()) \
                                          << QString.number(item.length()) \
                                          << QString.number(item.widthF()) \
                                          << QString.number(item.style()) \
                                          << item.color().name() \
                                          << QString("\n")
            elif isinstance(item, Rectangle):
                rect = item.rect()
                topLeft = rect.topLeft()
                stream << QString("Rectangle") << QString("%1,%2,%3,%4").arg(topLeft.x()).arg(topLeft.y()).arg(rect.width()).arg(rect.height()) \
                                               << QString("\n")
            elif isinstance(item, GraphicsProxyWidgetContainer):
                print "GraphicsProxyWidgetContainer"
            elif isinstance(item, GraphicsProxyWidget):
                print "GraphicsProxyWidget"
                embeddedWidget = item.widget()
                


    # The copied item data is extracted and the items are instantiated with the
    # refered binary data
    def paste(self):
        if self.__copiedItem.isEmpty():
            return
        
        stream = QDataStream(self.__copiedItem, QIODevice.ReadOnly)
        
        itemData = QStringList()
        while not stream.atEnd():
            input = QString()
            stream >> input
            
            if input == "\n":
                # Create item
                type = itemData.first()
                if type == "Text" and itemData.count() == 6:
                    textItem = Text(self.__isDesignMode)
                    textItem.setText(itemData[1])
                    font = QFont()
                    font.fromString(itemData[2])
                    textItem.setFont(font)
                    textItem.setTextColor(QColor(itemData[3]))
                    textItem.setOutlineColor(QColor(itemData[4]))
                    textItem.setBackgroundColor(QColor(itemData[5]))
                    self._setupItem(textItem)
                elif type == "Link":
                    print "Link"
                elif type == "Arrow":
                    print "Arrow"
                elif type == "Line" and itemData.count() == 6:
                    lineItem = Line(self.__isDesignMode)
                    # Get line coordinates
                    lineCoords = itemData[1].split(",")
                    if len(lineCoords) == 4:
                        line = QLineF(lineCoords[0].toFloat()[0], lineCoords[1].toFloat()[0], \
                                      lineCoords[2].toFloat()[0], lineCoords[3].toFloat()[0])
                        lineItem.setLine(line)
                    # Get line length
                    length = itemData[2].toFloat()
                    if length[1]:
                        lineItem.setLength(length[0])
                    # Get line width
                    widthF = itemData[3].toFloat()
                    if widthF[1]:
                        lineItem.setWidthF(widthF[0])
                    # Get line style
                    style = itemData[4].toInt()
                    if style[1]:
                        lineItem.setStyle(style[0])
                    # Get line color
                    lineItem.setColor(QColor(itemData[5]))
                    
                    self._setupItem(lineItem)
                    lineItem.setTransformOriginPoint(lineItem.boundingRect().center())
                elif type == "Rectangle" and itemData.count() == 2:
                    rectItem = Rectangle(self.__isDesignMode)
                    rectData = itemData[1].split(",")
                    if len(rectData) == 4:
                        rect = QRectF(rectData[0].toFloat()[0], rectData[1].toFloat()[0], \
                                      rectData[2].toFloat()[0], rectData[3].toFloat()[0])
                        rectItem.setRect(rect)
                    self._setupItem(rectItem)
                    rectItem.setTransformOriginPoint(rectItem.boundingRect().center())
                elif type == "GraphicsProxyWidgetContainer":
                    print "GraphicsProxyWidgetContainer"
                elif type == "GraphicsProxyWidget":
                    print "GraphicsProxyWidget"
                
                itemData.clear()
                continue
            
            itemData.append(input)


    # All selected items are removed; when an item (not type Link) is removed its
    # destructor deletes any links that are associated with it
    # To avoid double-deleting links, the Link-items are removed before deleting the other items
    def remove(self):
        items = self.selectedItems()
        if (len(items) and QMessageBox.question(self, "Remove selected items",
                                                "Remove {0} item{1}?".format(len(items),
                                                "s" if len(items) != 1 else ""),
                                                QMessageBox.Yes|QMessageBox.No) ==
                                                QMessageBox.No):
            return
        
        self.removeItems(items)


    def removeItems(self, items):
        while items:
            item = items.pop()
            if isinstance(item, Text) or isinstance(item, Rectangle):
                for link in item.links():
                    if link in items:
                        # Remove item from list - prevent double deletion
                        items.remove(link)
                    self.__scene.removeItem(link)
            elif isinstance(item, Link) or isinstance(item, Arrow):
                print "Link or Arrow removed"
            elif isinstance(item, Line):
                print "Line removed"
            elif isinstance(item, GraphicsProxyWidgetContainer):
                layout = item.layout()
                for i in xrange(layout.count()):
                    proxyItem = layout.itemAt(i)
                    proxyItem.destroy()
                    if proxyItem in items:
                        # Remove item from list - prevent double deletion
                        items.remove(proxyItem)
                    self.__scene.removeItem(proxyItem)
                item.destroy()
            elif isinstance(item, GraphicsProxyWidget):
                item.destroy()
            
            self.__scene.removeItem(item)
            del item


    # Rotates all selected items around 30 degrees
    def rotate(self):
        for item in self.selectedItems():
            #item.rotate(30)
            item.setRotation(self.__rotAngle)
            self.__rotAngle += 30


    # Scales all selected items up (so far) : TODO use value
    def scaleSelectedItems(self): # value
        for item in self.selectedItems():
            item.scale(1.5, 1.5)


    def groupItems(self):
        items = self.selectedItems()
        if len(items) < 1:
            return

        # Unselect all selected items
        for item in items:
            item.setSelected(False)

        itemGroup = self.__scene.createItemGroup(items)
        itemGroup.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable)
        itemGroup.setSelected(True)
        self.bringToFront()


    def horizontalLayout(self):
        items = self.selectedItems()
        if len(items) < 1:
            return
        self.createGraphicsItemContainer(Qt.Horizontal, items)


    def verticalLayout(self):
        items = self.selectedItems()
        if len(items) < 1:
            return
        self.createGraphicsItemContainer(Qt.Vertical, items)


    def breakLayout(self):
        items = self.selectedItems()
        if len(items) < 1:
            return
        for item in items:
            self.__scene.breakLayout(item)


    def unGroupItems(self):
        items = self.selectedItems()
        if len(items) < 1:
            return
        
        childItems = []
        for item in items:
            if isinstance(item, QGraphicsItemGroup):
                childItems = item.childItems()
                self.__scene.destroyItemGroup(item)
        # Select all items again
		for childItem in childItems:
		    childItem.setSelected(True)


    # Increments self.__maxZ value, and then sets the currently selected item's z
    # value to self.__maxZ
    def bringToFront(self):
        self.__maxZ += 1
        self._setZValue(self.__maxZ)

    
    # Decrements self.__minZ value, and then sets the currently selected item's z
    # value to self.__minZ
    def sendToBack(self):
        self.__minZ -=1
        self._setZValue(self.__minZ)


### private ###
    # Positions a newly added or pasted item in the scene
    # The sequence number ensures that new items are added in different positions
    # rather than on top of each other
    def _setupItem(self, item):
        item.setPos(QPointF(80 + (100 * (self.__seqNumber % 5)), 80 + (50 * ((self.__seqNumber / 5) % 7))))
        self.__seqNumber += 1
        self._addItem(item)


    def _addItem(self, item):
        self.__scene.addItem(item)
        self.__scene.clearSelection()
        item.setSelected(True)
        self.bringToFront()


    # Sets the z value of all selected items to z
    def _setZValue(self, z):
        items = self.selectedItems()
        for item in items:
            item.setZValue(z)


    # Creates and returns container item
    def createGraphicsItemContainer(self, orientation, items):
        # Initialize layout
        layout = QGraphicsLinearLayout(orientation)
        layout.setContentsMargins(5,5,5,5)
        
        width = 0
        height = 0
        for item in items:
            if isinstance(item, QGraphicsLayoutItem) is False:
                continue
            
            item.setFlag(QGraphicsItem.ItemIsMovable, False)
            item.setFlag(QGraphicsItem.ItemIsSelectable, False)
            
            layout.addItem(item)
            layout.setAlignment(item, Qt.AlignCenter)

            # Recalculate width and height of the whole item
            itemGeometry = item.geometry()
            width += itemGeometry.width()
            if height < itemGeometry.height():
                height = itemGeometry.height()

        # Create container item for items in layout
        containerItem = GraphicsProxyWidgetContainer(self.__isDesignMode)
        containerItem.setLayout(layout)
        # Set correct geometry for containerItem - important for positioning
        containerItem.setGeometry(QRectF(0, 0, width, height))
        # Add created item to scene
        self._addItem(containerItem)
        
        # Calculations to position item center-oriented
        bRect = containerItem.boundingRect()
        leftPos = bRect.topLeft()
        leftPos = containerItem.mapToScene(leftPos)
        centerPos = bRect.center()
        centerPos = containerItem.mapToScene(centerPos)
        offset = centerPos-leftPos
        containerItem.setTransformOriginPoint(centerPos)
        
        return (containerItem, offset)


### protected ###
    #def wheelEvent(self, event):
    #    #factor = 1.41 ** (-event.delta() / 240.0)
    #    factor = 1.0 + (0.2 * qAbs(event.delta()) / 120.0)
    #    if event.delta() > 0:
    #        self.scale(factor, factor)
    #    else:
    #        factor = 1.0/factor
    #        self.scale(factor, factor)
    #    QGraphicsView.wheelEvent(self, event)


    def mousePressEvent(self, event):
        if (event.button() != Qt.LeftButton):
            return

        # Items are created in origin and must then be moved to the position to
        # set their position correctly for later purposes!!!
        pos = QPointF(self.mapToScene(event.pos()))
        if self.__mode == self.InsertLine:
            self.__line = Line(self.__isDesignMode)
            self._addItem(self.__line)
            self.__line.setPos(pos.x(), pos.y())
        elif self.__mode == self.InsertRect:
            self.__rect = Rectangle(self.__isDesignMode)
            self._addItem(self.__rect)
            self.__rect.setPos(pos.x(), pos.y())

        QGraphicsView.mousePressEvent(self, event)


    def mouseMoveEvent(self, event):
        pos = self.mapToScene(event.pos())
        if self.__mode == self.InsertLine and self.__line:
            linePos = self.__line.pos()
            pos = QPointF(pos.x()-linePos.x(), pos.y()-linePos.y())
            newLine = QLineF(QPointF(), QPointF(pos))
            self.__line.setLine(newLine)
        elif self.__mode == self.InsertRect and self.__rect:
            rectPos = self.__rect.pos()
            pos = QPointF(pos.x()-rectPos.x(), pos.y()-rectPos.y())
            newRect = QRectF(QPointF(), QPointF(pos))
            self.__rect.setRect(newRect)
        #elif self.__mode == self.MoveItem:
        QGraphicsView.mouseMoveEvent(self, event)


    def mouseReleaseEvent(self, event):
        if self.__line and self.__mode == self.InsertLine:
            centerPos = self.__line.boundingRect().center()
            self.__line.setTransformOriginPoint(centerPos)
            self.__line.setSelected(True)
            self.lineInserted.emit()
        elif self.__rect and self.__mode == self.InsertRect:
            rect = self.__rect.boundingRect()
            centerPos = rect.center()
            self.__rect.setTransformOriginPoint(centerPos)
            self.__rect.setSelected(True)
            self.rectInserted.emit()

        self.__line = None
        self.__rect = None
        QGraphicsView.mouseReleaseEvent(self, event)


# Drag & Drop events
    def dragEnterEvent(self, event):
        #print "GraphicsView.dragEnterEvent"
        
        source = event.source()
        if (source is not None) and (source is not self):
            event.accept()
        
        QGraphicsView.dragEnterEvent(self, event)


    def dragMoveEvent(self, event):
        #print "GraphicsView.dragMoveEvent"
        event.accept()
        #QGraphicsView.dragMoveEvent(self, event)


    def dropEvent(self, event):
        #print "GraphicsView.dropEvent"
        
        source = event.source()
        if source is not None:
            customItem = None
            mimeData = event.mimeData()
            # Source type
            sourceType = mimeData.data("sourceType")
            # Drop from NavigationTreeView or AttributeTreeWidget?
            if sourceType == "NavigationTreeView":
                # Internal key
                internalKey = QString(mimeData.data("internalKey"))
                # Display name
                displayName = QString(mimeData.data("displayName"))
                customItem = GraphicsCustomItem(self.__isDesignMode, displayName)
                offset = QPointF()
                # Add created item to scene
                self._addItem(customItem)
            elif sourceType == "AttributeTreeWidget":
                # Internal key
                internalKey = QString(mimeData.data("internalKey"))
                # Display name
                displayName = QString(mimeData.data("displayName"))
                # Display component?
                hasDisplayComponent = mimeData.data("hasDisplayComponent").toInt()
                if hasDisplayComponent[1]:
                    hasDisplayComponent = hasDisplayComponent[0]
                # Editable component?
                hasEditableComponent = mimeData.data("hasEditableComponent").toInt()
                if hasEditableComponent[1]:
                    hasEditableComponent = hasEditableComponent[0]
                # Navigation item type
                navigationItemType = mimeData.data("navigationItemType").toInt()
                if navigationItemType[1]:
                    navigationItemType = navigationItemType[0]
                # Class alias
                classAlias = QString(mimeData.data("classAlias"))

                # List stored all items for layout
                items = []
                
                # Label
                label = QLabel(displayName)
                label.setAttribute(Qt.WA_NoSystemBackground, True)
                labelProxyWidget = GraphicsProxyWidget(self.__isDesignMode, label)
                labelProxyWidget.setTransformOriginPoint(labelProxyWidget.boundingRect().center())
                # Add item to itemlist
                items.append(labelProxyWidget)
                
                # Does key concern state of device?
                keys = str(internalKey).split('.', 1)
                isStateToDisplay = keys[1] == "state"
                
                # Display widget
                if hasDisplayComponent:
                    displayComponent = DisplayComponent(classAlias, key=internalKey)
                    displayComponent.widget.setAttribute(Qt.WA_NoSystemBackground, True)
                    displayProxyWidget = GraphicsProxyWidget(self.__isDesignMode, displayComponent.widget, displayComponent, isStateToDisplay)
                    displayProxyWidget.setTransformOriginPoint(displayProxyWidget.boundingRect().center())
                    # Add item to itemlist
                    items.append(displayProxyWidget)
                
                # Editable widget
                if hasEditableComponent:
                    if navigationItemType is NavigationItemTypes.DEVICE_CLASS:
                        editableComponent = EditableNoApplyComponent(classAlias, key=internalKey)
                    elif navigationItemType is NavigationItemTypes.DEVICE_INSTANCE:
                        editableComponent = EditableApplyLaterComponent(classAlias, key=internalKey)
                        editableComponent.isEditableValueInit = False
                    
                    editableComponent.widget.setAttribute(Qt.WA_NoSystemBackground, True)
                    editableProxyWidget = GraphicsProxyWidget(self.__isDesignMode, editableComponent.widget, editableComponent, isStateToDisplay)
                    editableProxyWidget.setTransformOriginPoint(editableProxyWidget.boundingRect().center())
                    # Add item to itemlist
                    items.append(editableProxyWidget)

                customTuple = self.createGraphicsItemContainer(Qt.Horizontal, items)
                customItem = customTuple[0]
                offset = customTuple[1]
            
            if customItem is None: return

            pos = event.pos()
            scenePos = self.mapToScene(pos)
            scenePos = scenePos-offset
            
            customItem.setPos(scenePos)

        event.accept()
        
        QGraphicsView.dropEvent(self, event)


    def _getWidgetCenterPosition(self, pos, centerX, centerY):
        # QPointF pos, int centerX, int centerY
        pos.setX(pos.x()-centerX)
        pos.setY(pos.y()-centerY)
        return pos


### slots ###
    def onRemoveUserCustomFrame(self, userCustomFrame):
        print "onRemoveUserCustomFrame", userCustomFrame
        if userCustomFrame is None:
            return
        userCustomFrame.deleteLater()


    # Called whenever an item of the scene is un-/selected
    def onSceneSelectionChanged(self):
        self.sceneSelectionChanged.emit()

