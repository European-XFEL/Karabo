#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents an XML writer for QGraphicsItems
   of the CustomWidget's GraphicsView/Scene."""

__all__ = ["CustomXmlWriter"]


from layoutcomponents.arrow import Arrow
from layoutcomponents.line import Line
from layoutcomponents.link import Link
from layoutcomponents.rectangle import Rectangle
from layoutcomponents.text import Text

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class CustomXmlWriter(QXmlStreamWriter):

    def __init__(self, scene):
        super(CustomXmlWriter, self).__init__()
        
        self.setAutoFormatting(True)
        self.__scene = scene


    def write(self, filename):
        # Open infoFile
        infoFile = QFile(filename)
        if infoFile.open(QIODevice.WriteOnly | QIODevice.Text) == False:
            raise Exception, "Creating the component project file failed!"

        self.setDevice(infoFile)
        
        # Write beginning of XML document
        self.writeStartDocument()
        self.writeDTD("<!DOCTYPE xfel.gui>")
        self.writeStartElement("SceneData")
        self.writeAttribute("version", "v0.1")

        self.writeStartElement("GraphicsItemList")
        
        for item in self.__scene.items():
            self.writeStartElement("GraphicsItem")
            
            if isinstance(item, Text):
                self.writeAttribute("type", "Text")
                self.writeAttribute("posX", QString.number(item.x()))
                self.writeAttribute("posY", QString.number(item.y()))
                
                # Save additional text item data
                self.writeTextElement("text", item.text())
                self.writeTextElement("font", item.font().toString())
                self.writeTextElement("textColor", item.textColor().name())
                self.writeTextElement("outlineColor", item.outlineColor().name())
                self.writeTextElement("backgroundColor", item.backgroundColor().name())
            elif isinstance(item, Link):
                self.writeAttribute("type", "Link")
                self.writeAttribute("posX", QString.number(item.x()))
                self.writeAttribute("posY", QString.number(item.y()))
            elif isinstance(item, Arrow):
                self.writeAttribute("type", "Arrow")
                self.writeAttribute("posX", QString.number(item.x()))
                self.writeAttribute("posY", QString.number(item.y()))
            elif isinstance(item, Line):
                self.writeAttribute("type", "Line")
                self.writeAttribute("posX", QString.number(item.x()))
                self.writeAttribute("posY", QString.number(item.y()))
                
                # Save additional line item data
                line = item.line()
                self.writeTextElement("lineCoords", QString("%1,%2,%3,%4").arg(line.x1()).arg(line.y1()).arg(line.x2()).arg(line.y2()))
                self.writeTextElement("length", QString.number(item.length()))
                self.writeTextElement("width", QString.number(item.widthF()))
                self.writeTextElement("style", QString.number(item.style()))
                self.writeTextElement("color", item.color().name())
            elif isinstance(item, Rectangle):
                self.writeAttribute("type", "Rectangle")
                self.writeAttribute("posX", QString.number(item.x()))
                self.writeAttribute("posY", QString.number(item.y()))
                
                # Save addition rectangle item data
                rect = item.rect()
                topLeft = rect.topLeft()
                self.writeTextElement("rectCoords", QString("%1,%2,%3,%4").arg(topLeft.x()).arg(topLeft.y()).arg(rect.width()).arg(rect.height()))
            
            self.writeEndElement() # End of GraphicsItem

        self.writeEndElement()  # End of GraphicsItemList  
        self.writeEndElement()  # End of SceneData

        self.writeEndDocument()
        infoFile.close()

