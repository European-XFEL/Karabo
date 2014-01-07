#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents an XML writer for QGraphicsItems
   of the CustomWidget's GraphicsView/Scene."""

__all__ = ["CustomXmlWriter"]

import displaycomponent
import widget
import editableapplylatercomponent
import editablenoapplycomponent

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
                    self._writeGraphicsProxyItem(item)
                    self.writeEndElement() # End of GraphicsItem
            else:
                self.writeStartElement("GraphicsItem")

                if isinstance(item, Text):
                    self.writeAttribute("type", "Text")
                    self.writeAttribute("posX","{}".format(item.x()))
                    self.writeAttribute("posY", "{}".format(item.y()))
                    self.writeAttribute("posZ", "{}".format(item.zValue()))

                    # Write item's transformation matrix
                    self._writeItemTransformation(item)

                    # Save additional text item data
                    self.writeTextElement("text", item.text())
                    self.writeTextElement("font", item.font().toString())
                    self.writeTextElement("textColor", item.textColor().name())
                    self.writeTextElement("outlineColor", item.outlineColor().name())
                    self.writeTextElement("backgroundColor", item.backgroundColor().name())

                elif isinstance(item, Link):
                    self.writeAttribute("type", "Link")
                    self.writeAttribute("posX", "{}".format(item.x()))
                    self.writeAttribute("posY", "{}".format(item.y()))
                    self.writeAttribute("posZ", "{}".format(item.zValue()))

                    # Write item's transformation matrix
                    self._writeItemTransformation(item)

                elif isinstance(item, Arrow):
                    self.writeAttribute("type", "Arrow")
                    self.writeAttribute("posX", "{}".format(item.x()))
                    self.writeAttribute("posY", "{}".format(item.y()))
                    self.writeAttribute("posZ", "{}".format(item.zValue()))

                    # Write item's transformation matrix
                    self._writeItemTransformation(item)

                elif isinstance(item, Line):
                    self.writeAttribute("type", "Line")
                    self.writeAttribute("posX", "{}".format(item.x()))
                    self.writeAttribute("posY", "{}".format(item.y()))
                    self.writeAttribute("posZ", "{}".format(item.zValue()))

                    # Write item's transformation matrix
                    self._writeItemTransformation(item)

                    # Save additional line item data
                    line = item.line()
                    self.writeTextElement("lineCoords", "{},{},{},{}".format(
                        line.x1(), line.y1(), line.x2(), line.y2()))
                    self.writeTextElement("length", "{}".format(item.length()))
                    self.writeTextElement("width", "{}".format(item.widthF()))
                    self.writeTextElement("style", "{}".format(item.style()))
                    self.writeTextElement("color", item.color().name())

                elif isinstance(item, Rectangle):
                    self.writeAttribute("type", "Rectangle")
                    self.writeAttribute("posX", "{}".format(item.x()))
                    self.writeAttribute("posY", "{}".format(item.y()))
                    self.writeAttribute("posZ", "{}".format(item.zValue()))

                    # Write item's transformation matrix
                    self._writeItemTransformation(item)

                    # Save addition rectangle item data
                    rect = item.rect()
                    topLeft = rect.topLeft()
                    self.writeTextElement("rectCoords", "{},{},{},{}".format(
                        topLeft.x(), topLeft.y(), rect.width(), rect.height()))

                elif isinstance(item, GraphicsProxyWidgetContainer):
                    self.writeAttribute("type", "GraphicsProxyWidgetContainer")
                    self.writeAttribute("posX", "{}".format(item.x()))
                    self.writeAttribute("posY", "{}".format(item.y()))
                    self.writeAttribute("posZ", "{}".format(item.zValue()))

                    # Write item's transformation matrix
                    self._writeItemTransformation(item)

                    layout = item.layout()
                    self.writeTextElement("layoutOrientation", 
                                          "{}".format(layout.orientation()))

                    nbItems =layout.count()
                    if nbItems > 0:
                        self.writeStartElement("GraphicsProxyItems")
                        for i in xrange(nbItems):
                            proxyItem = layout.itemAt(i)

                            if not proxyItem:
                                continue

                            self.writeStartElement("GraphicsItem")
                            # Do NOT write transformation matrix for item - False
                            self._writeGraphicsProxyItem(proxyItem, False)
                            self.writeEndElement() # End of GraphicsItem
                        self.writeEndElement() # End of GraphicsProxyItems
                elif isinstance(item, GraphicsCustomItem):
                    self.writeAttribute("type", "GraphicsCustomItem")
                    self.writeAttribute("posX", "{}".format(item.x()))
                    self.writeAttribute("posY", "{}".format(item.y()))
                    self.writeAttribute("posZ", "{}".format(item.zValue()))
                    
                    # Write item's transformation matrix
                    self._writeItemTransformation(item)
                    
                    self.writeTextElement("internalKey", item.internalKey())
                    self.writeTextElement("text", item.text())
                    self.writeTextElement("devInstId", item.value)
                
                self.writeEndElement() # End of GraphicsItem

        self.writeEndElement()  # End of GraphicsItemList  
        self.writeEndElement()  # End of SceneData

        self.writeEndDocument()
        infoFile.close()


    def _writeGraphicsProxyItem(self, proxyItem, writeTransform=True):
        
        component = proxyItem.component
        embeddedWidget = proxyItem.widget()

        if component is None and isinstance(embeddedWidget, QLabel):
            self.writeAttribute("posX", "{}".format(proxyItem.x()))
            self.writeAttribute("posY", "{}".format(proxyItem.y()))
            self.writeAttribute("posZ", "{}".format(proxyItem.zValue()))

            if writeTransform:
                # Write item's transformation matrix
                self._writeItemTransformation(proxyItem)

            self.writeTextElement("componentType", "Label")
            self.writeTextElement("widgetFactory", "Label")

            self.writeTextElement("Text", embeddedWidget.text())
        else:
            self.writeAttribute("posX", "{}".format(proxyItem.x()))
            self.writeAttribute("posY", "{}".format(proxyItem.y()))
            self.writeAttribute("posZ", "{}".format(proxyItem.zValue()))
            
            if writeTransform:
                # Write item's transformation matrix
                self._writeItemTransformation(proxyItem)
            
            widgetFactory = component.widgetFactory
            if isinstance(component, displaycomponent.DisplayComponent):
                self.writeTextElement("componentType", "DisplayComponent")
                if isinstance(widgetFactory, widget.DisplayWidget):
                    self.writeTextElement("widgetFactory", "DisplayWidget")
                elif isinstance(widgetFactory, widget.VacuumWidget):
                    self.writeTextElement("widgetFactory", "VacuumWidget")
            elif isinstance(component, editablenoapplycomponent.EditableNoApplyComponent):
                self.writeTextElement("componentType", "EditableNoApplyComponent")
                if isinstance(widgetFactory, widget.EditableWidget):
                    self.writeTextElement("widgetFactory", "EditableWidget")
            elif isinstance(component, editableapplylatercomponent.EditableApplyLaterComponent):
                self.writeTextElement("componentType", "EditableApplyLaterComponent")
                if isinstance(widgetFactory, widget.EditableWidget):
                    self.writeTextElement("widgetFactory", "EditableWidget")

            self.writeTextElement("classAlias", component.classAlias)
            
            if component.classAlias == "Command":
                self.writeTextElement("commandText", widgetFactory.widget.text())
                self.writeTextElement("commandEnabled", "{}".format(widgetFactory.widget.isEnabled()))
                self.writeTextElement("allowedStates", ",".join(widgetFactory.allowedStates))
                self.writeTextElement("command", widgetFactory.command)
                 
            
            keyString = str()
            nbKeys = len(component.keys)
            for i in xrange(nbKeys):
                keyString += component.keys[i]
                if i != (nbKeys-1):
                    keyString += ","
            self.writeTextElement("internalKeys", keyString)


    def _writeItemTransformation(self, item):
        transform = item.sceneTransform()
        
        # Save tranformation of item
        self.writeStartElement("sceneTransformation")
        self.writeTextElement("m11", "{}".format(transform.m11()))
        self.writeTextElement("m12", "{}".format(transform.m12()))
        self.writeTextElement("m13", "{}".format(transform.m13()))

        self.writeTextElement("m21", "{}".format(transform.m21()))
        self.writeTextElement("m22", "{}".format(transform.m22()))
        self.writeTextElement("m23", "{}".format(transform.m23()))

        self.writeTextElement("m31", "{}".format(transform.m31()))
        self.writeTextElement("m32", "{}".format(transform.m32()))
        self.writeTextElement("m33", "{}".format(transform.m33()))

        self.writeEndElement()  # End of TransformMatrix

