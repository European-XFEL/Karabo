#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents an XML writer for QGraphicsItems
   of the CustomWidget's GraphicsView/Scene."""

__all__ = ["CustomXmlWriter"]

import displaycomponent
import displaywidget
import editablewidget
import editableapplylatercomponent
import editablenoapplycomponent
import vacuumwidget

from layoutcomponents.arrow import Arrow
from layoutcomponents.graphicscustomitem import GraphicsCustomItem
from layoutcomponents.graphicsproxywidget import GraphicsProxyWidget
from layoutcomponents.graphicsproxywidgetcontainer import GraphicsProxyWidgetContainer
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
            QMessageBox.critical(None, "Save scene", "Creating the component project file failed!")
            raise Exception, "Creating the component project file failed!"

        self.setDevice(infoFile)
        
        # Write beginning of XML document
        self.writeStartDocument()
        self.writeDTD("<!DOCTYPE xfel.gui>")
        self.writeStartElement("SceneData")
        self.writeAttribute("version", "v0.1")

        self.writeStartElement("GraphicsItemList")
        
        for item in self.__scene.items():
            
            if isinstance(item, GraphicsProxyWidget):
                if item.parentWidget() is None:
                    self.writeStartElement("GraphicsItem")
                    
                    self.writeAttribute("type", "GraphicsProxyWidget")
                    self.writeGraphicsProxyItem(item)
                    
                    self.writeEndElement() # End of GraphicsItem
            else:
                self.writeStartElement("GraphicsItem")

                if isinstance(item, Text):
                    self.writeAttribute("type", "Text")
                    self.writeAttribute("posX", QString.number(item.x()))
                    self.writeAttribute("posY", QString.number(item.y()))
                    self.writeAttribute("posZ", QString.number(item.zValue()))

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
                    self.writeAttribute("posZ", QString.number(item.zValue()))
                elif isinstance(item, Arrow):
                    self.writeAttribute("type", "Arrow")
                    self.writeAttribute("posX", QString.number(item.x()))
                    self.writeAttribute("posY", QString.number(item.y()))
                    self.writeAttribute("posZ", QString.number(item.zValue()))
                elif isinstance(item, Line):
                    self.writeAttribute("type", "Line")
                    self.writeAttribute("posX", QString.number(item.x()))
                    self.writeAttribute("posY", QString.number(item.y()))
                    self.writeAttribute("posZ", QString.number(item.zValue()))

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
                    self.writeAttribute("posZ", QString.number(item.zValue()))

                    # Save addition rectangle item data
                    rect = item.rect()
                    topLeft = rect.topLeft()
                    self.writeTextElement("rectCoords", QString("%1,%2,%3,%4").arg(topLeft.x()).arg(topLeft.y()).arg(rect.width()).arg(rect.height()))
                elif isinstance(item, GraphicsProxyWidgetContainer):
                    self.writeAttribute("type", "GraphicsProxyWidgetContainer")
                    self.writeAttribute("posX", QString.number(item.x()))
                    self.writeAttribute("posY", QString.number(item.y()))
                    self.writeAttribute("posZ", QString.number(item.zValue()))

                    layout = item.layout()
                    self.writeTextElement("layoutOrientation", QString.number(layout.orientation()))

                    nbItems =layout.count()
                    if nbItems > 0:
                        self.writeStartElement("GraphicsProxyItems")
                        for i in xrange(nbItems):
                            proxyItem = layout.itemAt(i)

                            if not proxyItem:
                                continue

                            self.writeStartElement("GraphicsItem")
                            self.writeGraphicsProxyItem(proxyItem)
                            self.writeEndElement() # End of GraphicsItem
                        self.writeEndElement() # End of GraphicsProxyItems
                elif isinstance(item, GraphicsCustomItem):
                    self.writeAttribute("type", "GraphicsCustomItem")
                    self.writeAttribute("posX", QString.number(item.x()))
                    self.writeAttribute("posY", QString.number(item.y()))
                    self.writeAttribute("posZ", QString.number(item.zValue()))
                    
                    self.writeTextElement("internalKey", item.internalKey())
                    self.writeTextElement("text", item.text())
                    self.writeTextElement("additionalText", item.additionalText())
                                
                self.writeEndElement() # End of GraphicsItem

        self.writeEndElement()  # End of GraphicsItemList  
        self.writeEndElement()  # End of SceneData

        self.writeEndDocument()
        infoFile.close()


    def writeGraphicsProxyItem(self, proxyItem):
        
        component = proxyItem.component
        embeddedWidget = proxyItem.widget()

        if component is None and isinstance(embeddedWidget, QLabel):
            self.writeAttribute("posX", QString.number(proxyItem.x()))
            self.writeAttribute("posY", QString.number(proxyItem.y()))
            self.writeAttribute("posZ", QString.number(proxyItem.zValue()))

            self.writeTextElement("componentType", "Label")
            self.writeTextElement("widgetFactory", "Label")

            self.writeTextElement("Text", embeddedWidget.text())
        else:
            self.writeAttribute("posX", QString.number(proxyItem.x()))
            self.writeAttribute("posY", QString.number(proxyItem.y()))
            self.writeAttribute("posZ", QString.number(proxyItem.zValue()))
            
            widgetFactory = component.widgetFactory
            if isinstance(component, displaycomponent.DisplayComponent):
                self.writeTextElement("componentType", "DisplayComponent")
                if isinstance(widgetFactory, displaywidget.DisplayWidget):
                    self.writeTextElement("widgetFactory", "DisplayWidget")
                elif isinstance(widgetFactory, vacuumwidget.VacuumWidget):
                    self.writeTextElement("widgetFactory", "VacuumWidget")
            elif isinstance(component, editablenoapplycomponent.EditableNoApplyComponent):
                self.writeTextElement("componentType", "EditableNoApplyComponent")
                if isinstance(widgetFactory, editablewidget.EditableWidget):
                    self.writeTextElement("widgetFactory", "EditableWidget")
            elif isinstance(component, editableapplylatercomponent.EditableApplyLaterComponent):
                self.writeTextElement("componentType", "EditableApplyLaterComponent")
                if isinstance(widgetFactory, editablewidget.EditableWidget):
                    self.writeTextElement("widgetFactory", "EditableWidget")

            self.writeTextElement("classAlias", component.classAlias)
            keyString = str()
            nbKeys = len(component.keys)
            for i in xrange(nbKeys):
                keyString += component.keys[i]
                if i != (nbKeys-1):
                    keyString += ","
            self.writeTextElement("internalKeys", keyString)

