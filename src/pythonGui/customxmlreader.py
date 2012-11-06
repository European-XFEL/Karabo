#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents an XML reader for QGraphicsItems
   of the CustomWidget's GraphicsView/Scene."""

__all__ = ["CustomXmlReader"]


from displaycomponent import DisplayComponent

from editableapplylatercomponent import EditableApplyLaterComponent
from editablenoapplycomponent import EditableNoApplyComponent

from layoutcomponents.arrow import Arrow
from layoutcomponents.graphicsproxywidget import GraphicsProxyWidget
from layoutcomponents.graphicsproxywidgetcontainer import GraphicsProxyWidgetContainer
from layoutcomponents.line import Line
from layoutcomponents.link import Link
from layoutcomponents.rectangle import Rectangle
from layoutcomponents.text import Text

from manager import Manager

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class CustomXmlReader(QXmlStreamReader):

    def __init__(self, view):
        super(CustomXmlReader, self).__init__()
        
        self.__view = view


    def read(self, data):
        self.clear()
        self.addData(data)

        self.readNext() # StartDocument
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "GraphicsItemList":
                    self.processItems()
            elif tokenType == QXmlStreamReader.EndElement:
                break


    def processItems(self):
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "GraphicsItem":
                    type, posX, posY, posZ = self._processItemAttributes()
                    
                    item = None
                    if type == "Text":
                        item = Text(self.__view.isDesignMode)
                        self.__view.scene().addItem(item)
                        item.setPos(QPointF(posX, posY))
                        item.setZValue(posZ)
                        self._processGraphicsItem(item)
                    elif type == "Link":
                        item = Link()
                        self.__view.scene().addItem(item)
                        item.setPos(QPointF(posX, posY))
                        item.setZValue(posZ)
                        self._processGraphicsItem(item)
                    elif type == "Arrow":
                        item = Arrow()
                        self.__view.scene().addItem(item)
                        item.setPos(QPointF(posX, posY))
                        item.setZValue(posZ)
                        self._processGraphicsItem(item)
                    elif type == "Line":
                        item = Line(self.__view.isDesignMode)
                        self.__view.scene().addItem(item)
                        item.setPos(QPointF(posX, posY))
                        item.setZValue(posZ)
                        self._processGraphicsItem(item)
                    elif type == "Rectangle":
                        item = Rectangle(self.__view.isDesignMode)
                        self.__view.scene().addItem(item)
                        item.setPos(QPointF(posX, posY))
                        item.setZValue(posZ)
                        self._processGraphicsItem(item)
                    elif type == "GraphicsProxyWidgetContainer":
                        self._processGraphicsProxyWidgetContainer(posX, posY, posZ)
                    elif type == "GraphicsProxyWidget":
                        item = self._processGraphicsProxyWidget(posX, posY, posZ)
                        item.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable)
                        self.__view.scene().addItem(item)
            elif tokenType == QXmlStreamReader.EndElement and tagName == "GraphicsItemList":
                break


    def _processItemAttributes(self):
        type = self.attributes().value("type").toString()
        
        posX = self.attributes().value("posX").toString().toDouble()
        posY = self.attributes().value("posY").toString().toDouble()
        posZ = self.attributes().value("posZ").toString().toDouble()
        
        return [type, posX[0], posY[0], posZ[0]]


    def _processGraphicsItem(self, item):
        if item is None:
            return
        
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()
            if tokenType == QXmlStreamReader.StartElement:
                if isinstance(item, Text):
                    if tagName == "text":
                        self.readNext()
                        item.setText(self.text().toString())
                    elif tagName == "font":
                        self.readNext()
                        font = QFont()
                        font.fromString(self.text().toString())
                        item.setFont(font)
                    elif tagName == "textColor":
                        self.readNext()
                        item.setTextColor(QColor(self.text().toString()))
                    elif tagName == "outlineColor":
                        self.readNext()
                        item.setOutlineColor(QColor(self.text().toString()))
                    elif tagName == "backgroundColor":
                        self.readNext()
                        item.setBackgroundColor(QColor(self.text().toString()))
                elif isinstance(item, Link):
                    pass
                elif isinstance(item, Arrow):
                    pass
                elif isinstance(item, Line):
                    if tagName == "lineCoords":
                        self.readNext()
                        linePositions = self.text().toString().split(",")
                        if len(linePositions) == 4:
                            line = QLineF(linePositions[0].toFloat()[0], linePositions[1].toFloat()[0], \
                                          linePositions[2].toFloat()[0], linePositions[3].toFloat()[0])
                            item.setLine(line)
                    elif tagName == "length":
                        self.readNext()
                        length = self.text().toString().toFloat()
                        if length[1]:
                            item.setLength(length[0])
                    elif tagName == "width":
                        self.readNext()
                        widthF = self.text().toString().toFloat()
                        if widthF[1]:
                            item.setWidthF(widthF[0])
                    elif tagName == "style":
                        self.readNext()
                        style = self.text().toString().toInt()
                        if style[1]:
                            item.setStyle(style[0])
                    elif tagName == "color":
                        self.readNext()
                        item.setColor(QColor(self.text().toString()))
                elif isinstance(item, Rectangle):
                    if tagName == "rectCoords":
                        self.readNext()
                        rectData = self.text().toString().split(",")
                        if len(rectData) == 4:
                            rect = QRectF(rectData[0].toFloat()[0], rectData[1].toFloat()[0], \
                                          rectData[2].toFloat()[0], rectData[3].toFloat()[0])
                            item.setRect(rect)
                    
            elif tokenType == QXmlStreamReader.EndElement and tagName == "GraphicsItem":
                break


    def _processGraphicsProxyWidgetContainer(self, posX, posY, posZ):
        
        layoutOrientation = None
        
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()
            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "layoutOrientation":
                    self.readNext()
                    layoutOrientation = self.text().toString().toInt()
                    if layoutOrientation[1]:
                        layoutOrientation = layoutOrientation[0]
                elif tagName == "GraphicsProxyItems":
                    self._processGraphicsProxyItemList(layoutOrientation, posX, posY, posZ)
                    
            elif tokenType == QXmlStreamReader.EndElement and tagName == "GraphicsItem":
                break


    def _processGraphicsProxyItemList(self, orientation, posX, posY, posZ):

        proxyItems = []
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()
            
            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "GraphicsItem":
                    self.readNext()
                    type, itemPosX, itemPosY, itemPosZ = self._processItemAttributes()
                    proxyItems.append(self._processGraphicsProxyWidget(itemPosX, itemPosY, itemPosZ))
            
            elif tokenType == QXmlStreamReader.EndElement and tagName == "GraphicsProxyItems":
                break
        
        customTuple = self.__view.createGraphicsItemContainer(orientation, proxyItems)
        container = customTuple[0]
        #offset = customTuple[1]
        
        #pos = QPoint(posX, posY)
        #scenePos = self.__view.mapToScene(pos)
        #scenePos = scenePos-offset
        #container.setPos(scenePos)
        container.setPos(QPointF(posX, posY))
        container.setZValue(posZ)


    def _processGraphicsProxyWidget(self, posX, posY, posZ):
        
        componentType = None
        widgetFactory = None
        text = None
        classAlias = None
        internalKeys = None
        
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()
    
            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "componentType":
                    self.readNext()
                    componentType = self.text().toString()
                elif tagName == "widgetFactory":
                    self.readNext()
                    widgetFactory = self.text().toString()
                elif tagName == "Text":
                    self.readNext()
                    text = self.text().toString()
                elif tagName == "classAlias":
                    self.readNext()
                    classAlias = self.text().toString()
                elif tagName == "internalKeys":
                    self.readNext()
                    internalKeys = self.text().toString()
            
            elif tokenType == QXmlStreamReader.EndElement and tagName == "GraphicsItem":
                break
        
        #print ""
        #print componentType, widgetFactory, text, classAlias, internalKeys
        #print ""
        
        internalKey = None
        isStateToDisplay = False
        if internalKeys is not None:
            internalKey = str(internalKeys).split(',')
            
            if len(internalKey) == 1:
                internalKey = internalKey[0]

            # Does key concern state of device?
            keys = str(internalKey).split('.', 1)
            isStateToDisplay = (keys[1] == "state")
        
        proxyItem = None
        # Create GraphicsProxyWidget
        if componentType == "Label":
            # Label
            label = QLabel(text)
            label.setAttribute(Qt.WA_NoSystemBackground, True)
            proxyItem = GraphicsProxyWidget(self.__view.isDesignMode, label)
            proxyItem.setTransformOriginPoint(proxyItem.boundingRect().center())
        elif componentType == "DisplayComponent":
            # Display value
            #displayValue = item.displayComponent.value

            displayComponent = DisplayComponent(classAlias, key=internalKey, widgetFactory=widgetFactory)
            displayComponent.widget.setAttribute(Qt.WA_NoSystemBackground, True)
            proxyItem = GraphicsProxyWidget(self.__view.isDesignMode, displayComponent.widget, displayComponent, isStateToDisplay)
            proxyItem.setTransformOriginPoint(proxyItem.boundingRect().center())
        elif componentType == "EditableNoApplyComponent":
            # Editable value
            #editableValue = item.editableComponent.value

            editableComponent = EditableNoApplyComponent(classAlias, key=internalKey, widgetFactory=widgetFactory)
            editableComponent.widget.setAttribute(Qt.WA_NoSystemBackground, True)
            proxyItem = GraphicsProxyWidget(self.__view.isDesignMode, editableComponent.widget, editableComponent, isStateToDisplay)
            proxyItem.setTransformOriginPoint(proxyItem.boundingRect().center())
        elif componentType == "EditableApplyLaterComponent":
            # Editable value
            #editableValue = item.editableComponent.value
            
            editableComponent = EditableApplyLaterComponent(classAlias, key=internalKey, widgetFactory=widgetFactory)
            editableComponent.isEditableValueInit = False
            editableComponent.widget.setAttribute(Qt.WA_NoSystemBackground, True)
            proxyItem = GraphicsProxyWidget(self.__view.isDesignMode, editableComponent.widget, editableComponent, isStateToDisplay)
            proxyItem.setTransformOriginPoint(proxyItem.boundingRect().center())
        
        Manager().onRefreshInstance(internalKey)
        
        proxyItem.setPos(QPointF(posX, posY))
        proxyItem.setZValue(posZ)
        
        return proxyItem

