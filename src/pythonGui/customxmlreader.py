#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents an XML reader for QGraphicsItems
   of the CustomWidget's GraphicsView/Scene."""

__all__ = ["CustomXmlReader"]

from layoutcomponents.arrow import Arrow
from layoutcomponents.line import Line
from layoutcomponents.link import Link
from layoutcomponents.rectangle import Rectangle
from layoutcomponents.text import Text

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class CustomXmlReader(QXmlStreamReader):

    def __init__(self, scene, isEditableMode):
        super(CustomXmlReader, self).__init__()
        
        self.__scene = scene
        self.__isEditableMode = isEditableMode


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
                    type, posX, posY = self.processItemAttributes()
                    
                    item = None
                    if type == "Text":
                        item = Text(self.__isEditableMode)
                        self.__scene.addItem(item)
                        item.setPos(QPointF(posX, posY))
                    elif type == "Link":
                        item = Link()
                        self.__scene.addItem(item)
                        item.setPos(QPointF(posX, posY))
                    elif type == "Arrow":
                        item = Arrow()
                        self.__scene.addItem(item)
                        item.setPos(QPointF(posX, posY))
                    elif type == "Line":
                        item = Line(self.__isEditableMode)
                        self.__scene.addItem(item)
                        item.setPos(QPointF(posX, posY))
                    elif type == "Rectangle":
                        item = Rectangle(self.__isEditableMode)
                        self.__scene.addItem(item)
                        item.setPos(QPointF(posX, posY))
                    
                    self.processGraphicsItem(item)
            elif tokenType == QXmlStreamReader.EndElement and tagName == "GraphicsItemList":
                break


    def processItemAttributes(self):
        type = self.attributes().value("type").toString()
        
        posX = self.attributes().value("posX").toString().toDouble()
        posY = self.attributes().value("posY").toString().toDouble()
        
        return [type, posX[0], posY[0]]


    def processGraphicsItem(self, item):
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

