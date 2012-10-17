#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 25, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the area in the middle where
   the user can drag and drop either items of the navigation panel or items of
   the configuration panel to get a user specific view of certain properties.
   
   This widget is embedded in the CustomMiddlePanel.
"""

__all__ = ["CustomWidget"]

from customxmlreader import CustomXmlReader
from customxmlwriter import CustomXmlWriter
from graphicsview import GraphicsView
from layoutcomponents.arrow import Arrow
from layoutcomponents.line import Line
from layoutcomponents.link import Link
from layoutcomponents.linkbase import LinkBase
from layoutcomponents.rectangle import Rectangle
from layoutcomponents.text import Text
from layoutcomponents.textdialog import TextDialog

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class CustomWidget(QWidget):
    # Signals
    lineInserted = pyqtSignal()
    rectInserted = pyqtSignal()
    sceneSelectionChanged = pyqtSignal()
    
    def __init__(self, parent):
        super(CustomWidget, self).__init__(parent)

        self.__scene = QGraphicsScene(0, 0, 600, 500)
        self.__scene.selectionChanged.connect(self.onSceneSelectionChanged)
        
        self.__view = GraphicsView()
        self.__view.setScene(self.__scene)
        self.__view.lineInserted.connect(self.onLineInserted)
        self.__view.rectInserted.connect(self.onRectInserted)
        
        self.__minZ = 0
        self.__maxZ = 0
        self.__seqNumber = 0
        
        self.__rotAngle = 30
        
        # Describes most recent item to be cut or copied inside the application
        self.__copiedItem = QByteArray()
        
        self.__slZoom = QSlider(Qt.Horizontal)
        self.__slZoom.setRange(5, 200)
        self.__slZoom.setValue(100)
        self.__slZoom.valueChanged.connect(self.onZoom)
        hZoomLayout = QHBoxLayout()
        hZoomLayout.addStretch()
        hZoomLayout.addWidget(self.__slZoom)
        
        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(0,0,0,0)
        vLayout.addWidget(self.__view)
        vLayout.addLayout(hZoomLayout)


### protected ###
    def keyPressEvent(self, event):
        if event.key() == Qt.Key_Delete:
            self.remove()
        else:
            event.ignore()
        QWidget.keyPressEvent(self, event)


### public functions ###
    def _getScene(self):
        return self.__scene
    scene = property(fget=_getScene)


    def setEditableMode(self, isEditableMode):
        self.__view.setEditableMode(isEditableMode)


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


    # Returns true, when items has been copied; otherwise false
    def hasCopy(self):
        return not self.__copiedItem.isEmpty()


    # Open saved view from file
    def openSceneFromFile(self):
        filename = QFileDialog.getOpenFileName(None, "Open Saved Configuration", QDir.tempPath(), "SCENE (*.scene)")
        if filename.isEmpty():
            return
        
        file = QFile(filename)
        if file.open(QIODevice.ReadOnly | QIODevice.Text) == False:
            return
        
        xmlContent = str()
        while file.atEnd() == False:
            xmlContent += str(file.readLine())
        
        self.__scene.clear()
        CustomXmlReader(self.__scene).read(xmlContent)


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
        
        textItem = Text(self.__view.isEditableMode)
        textItem.setText(textDialog.text())
        textItem.setFont(textDialog.font())
        textItem.setTextColor(textDialog.textColor())
        textItem.setBackgroundColor(textDialog.backgroundColor())
        textItem.setOutlineColor(textDialog.outlineColor())
        
        #node.setText(QString("Node %1").arg(self.__seqNumber + 1))
        self._setupItem(textItem)


    # Add a line
    def aboutToInsertLine(self, checked):
        if checked:
            self.__view.mode = GraphicsView.InsertLine
        else:
            self.__view.mode = GraphicsView.MoveItem


    # Add a rectangle
    def aboutToInsertRect(self, checked):
        if checked:
            self.__view.mode = GraphicsView.InsertRect
        else:
            self.__view.mode = GraphicsView.MoveItem


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
                    textItem = Text()
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
                    lineItem = Line()
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
                    rectItem = Rectangle()
                    rectData = itemData[1].split(",")
                    if len(rectData) == 4:
                        rect = QRectF(rectData[0].toFloat()[0], rectData[1].toFloat()[0], \
                                      rectData[2].toFloat()[0], rectData[3].toFloat()[0])
                        rectItem.setRect(rect)
                    self._setupItem(rectItem)
                    rectItem.setTransformOriginPoint(rectItem.boundingRect().center())
                
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
        
        while items:
            item = items.pop()
            if isinstance(item, Text) or isinstance(item, Rectangle):
                for link in item.links():
                    if link in items:
                        items.remove(link)
                    self.__scene.removeItem(link)
            elif isinstance(item, Link) or isinstance(item, Arrow):
                print "Link or Arrow removed"
            elif isinstance(item, Line):
                print "Line removed"
            
            self.__scene.removeItem(item)
            del item
        

    # Rotates all selected items around 30 degrees
    def rotate(self):
        for item in self.selectedItems():
            #item.rotate(30)
            item.setRotation(self.__rotAngle)
            self.__rotAngle += 30


    # Scales all selected items up (so far) : TODO use value
    def scale(self): # value
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
        self.__scene.addItem(item)
        self.__seqNumber += 1
        self.__scene.clearSelection()
        item.setSelected(True)
        self.bringToFront()


    # Sets the z value of all selected items to z
    def _setZValue(self, z):
        items = self.selectedItems()
        for item in items:
            item.setZValue(z)


### private slots ###
    # Called whenever the user rolls the mouse wheel, and it will cause the view
    # to scale smaller or larger depending on which way the wheel is rolled
    def onZoom(self, value):
        factor = value / 100.0
        print "onZoom", factor
        matrix = self.__view.matrix()
        matrix.reset()
        matrix.scale(factor, factor)
        self.__view.setMatrix(matrix)


    # Called whenever an item of the scene is un-/selected
    def onSceneSelectionChanged(self):
        self.sceneSelectionChanged.emit()


    # Called whenever a new line item was inserted in the scene
    def onLineInserted(self):
        self.lineInserted.emit()


    # Called whenever a new line item was inserted in the scene
    def onRectInserted(self):
        self.rectInserted.emit()

