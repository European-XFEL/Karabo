#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 19, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which is a customwidget
   component for the middle panel."""

__all__ = ["GraphicsCustomItem", "PrivateSchemaReader"]


from karabo.karathon import *

from graphicsinputchannelitem import GraphicsInputChannelItem
from graphicsoutputchannelitem import GraphicsOutputChannelItem

from layoutcomponents.nodebase import NodeBase

from manager import Manager

from registry import Loadable, ns_karabo, ns_svg

from PyQt4.QtCore import *
from PyQt4.QtGui import *

from xml.etree import ElementTree


class GraphicsCustomItem(NodeBase, Loadable, QObject):
    # signals
    signalValueChanged = pyqtSignal(str, object) # key, value


    def __init__(self, internalKey, isDesignMode, text, schema, showAdditionalInfo=True):
        super(GraphicsCustomItem, self).__init__(isDesignMode)
        
        self.__textFont = QFont()
        self.__text = text
        
        self.__showAdditionalInfo = showAdditionalInfo
        
        self.__deviceId = ""
        
        self.__compositeText = None
        self._updateCompositeText()
        
        self.__internalKey = internalKey
        
        self.__inputChannelData = [] # List contains Hashes with input data
        self.__outputChannelData = [] # List contains Hashes with output data
        
        self.__inputChannelItems = [] # List contains GraphicsInputChannelItem
        self.__outputChannelItems = [] # List contains GraphicsOutputChannelItem
        self.position = QPoint()
        self.selected = False
        
        if schema:
            schemaReader = PrivateSchemaReader(schema)
            # Get information to draw possible in/output channels
            self.__inputChannelData = schemaReader.inputChannelData
            self.__outputChannelData = schemaReader.outputChannelData
            # Update channel graphics representation
            self._updateChannelItems()

    def internalKey(self):
        return self.__internalKey

    def setToolTip(self, tip):
        pass


    def translate(self, pos):
        self.position += pos


    def _getValue(self):
        return self.__deviceId
    def _setValue(self, value):
        if self.__deviceId == value:
            return
        
        # Prepare item for change
        self.prepareGeometryChange()
        # Prepare channel item for change
        self._prepareGeometryChangeChannelItems()
        
        self.__deviceId = value
        self._updateCompositeText()
        
        # Update item geometry
        self.update()
        # Update channel item geometry
        self._updateChannelItems()
        
        # Send changing signal to Manager (connected only for DEVICE_CLASS)
        self.signalValueChanged.emit(self.deviceIdKey, self.__deviceId)
    value = property(fget=_getValue, fset=_setValue)


    def _getDeviceIdKey(self):
        return self.__internalKey + ".configuration.deviceId"
    deviceIdKey = property(fget=_getDeviceIdKey)


    def text(self):
        return self.__text


    def inputChannelItems(self):
        return self.__inputChannelItems


    def outputChannelItems(self):
        return self.__outputChannelItems


    def shape(self):
        rect = self._outlineRect()
        path = QPainterPath()
        path.addRoundRect(rect, self._roundness(rect.width()), self._roundness(rect.height()))
        return path

    def contains(self, pos):
        return self._outlineRect().contains(pos)

    def geometry(self):
        return self._outlineRect()


    def draw(self, painter):
        pen = painter.pen()
        if self.selected:
            pen.setStyle(Qt.DotLine)
            pen.setWidth(2)
        
        painter.setFont(self.__textFont)
        painter.setPen(pen)
        painter.setBrush(QColor(224,240,255)) # light blue
        rect = self._outlineRect()
        painter.drawRoundRect(rect, self._roundness(rect.width()), self._roundness(rect.height()))
        painter.drawText(rect, Qt.AlignCenter, self.__compositeText)


### private ###
    def _updateCompositeText(self):
        if self.__showAdditionalInfo:
            self.__compositeText = str(self.__text + "\n<" + self.__deviceId + ">")
        else:
            self.__compositeText = self.__text


    # Prepare channel items for geometry change
    def _prepareGeometryChangeChannelItems(self):
        for inputChannel in self.__inputChannelItems:
            inputChannel.prepareGeometryChange()
        
        for outputChannel in self.__outputChannelItems:
            outputChannel.prepareGeometryChange()


    # Update channel items
    def _updateChannelItems(self):
        rect = self._outlineRect()
        width = rect.width()
        height = rect.height()

        # Updating input channel items
        nbInputChannels = len(self.__inputChannelData)
        nbInputChannelItems = len(self.__inputChannelItems)
        x = -width/2
        yDelta = height / (nbInputChannels+1)
        for i in xrange(nbInputChannels):
            hash = self.__inputChannelData[i]
            y = yDelta * i
            if nbInputChannels > 1:
                y -= (yDelta/2)
            
            if nbInputChannelItems == 0:
                if hash.has("key"):
                    key = hash.get("key")
                else:
                    key = None
                if hash.has("type"):
                    type = hash.get("type")
                else:
                    type = None
                inputChannelItem = GraphicsInputChannelItem(self, key, type)
                self.__inputChannelItems.append(inputChannelItem)
            else:
                inputChannelItem = self.__inputChannelItems[i]
            inputChannelItem.setPos(QPointF(x-inputChannelItem.boundingRect().width(), y))

        # Updating output channel items
        nbOutputChannels = len(self.__outputChannelData)
        nbOutputChannelItems = len(self.__outputChannelItems)
        x = width/2
        yDelta = height / (nbOutputChannels+1)
        for i in xrange(nbOutputChannels):
            hash = self.__outputChannelData[i]
            y = yDelta * i
            if nbOutputChannels > 1:
                y -= (yDelta/2)
            
            if nbOutputChannelItems == 0:
                if hash.has("key"):
                    key = hash.get("key")
                else:
                    key = None
                if hash.has("type"):
                    type = hash.get("type")
                else:
                    type = None
                outputChannelItem = GraphicsOutputChannelItem(self, key, type)
                self.__outputChannelItems.append(outputChannelItem)
            else:
                outputChannelItem = self.__outputChannelItems[i]
            outputChannelItem.setPos(QPointF(x, y))
            outputChannelItem.predefinedDevInstId = self.__deviceId


    def _outlineRect(self):
        padding = 10
        metrics = QFontMetrics(self.__textFont) #qApp.fontMetrics())
        rect = metrics.boundingRect(self.__compositeText)
        rect.adjust(-padding, -padding, +padding, +padding)
        rect.translate(-rect.center())
        rect.translate(self.position)
        return rect


    def _roundness(self, size):
        diameter = 6
        return 100 * diameter / int(size)


### protected ###
    def mouseDoubleClickEvent(self, event):
        additionalText = QInputDialog.getText(event.widget(), "Edit device instance", \
                                              "Enter new device instance:", QLineEdit.Normal, \
                                              self.__deviceId)
        
        if len(additionalText[0]) > 0:
            self.value = additionalText[0]

        QGraphicsObject.mouseDoubleClickEvent(self, event)


    def itemChange(self, change, value):
        if change == QGraphicsItem.ItemPositionHasChanged: # ItemPositionChange
            for inputChannel in self.__inputChannelItems:
                inputChannel.trackChannelConnectionItems()

            for outputChannel in self.__outputChannelItems:
                outputChannel.trackChannelConnectionItems()
        return QGraphicsItem.itemChange(self, change, value)


### slots ###
    # Triggered by DataNotifier signalUpdateComponent
    def onValueChanged(self, key, value):
        if self.deviceIdKey == key:
            self.value = value


    def element(self):
        g = self.geometry()
        d = { "x": g.x(), "y": g.y(), "width": g.width(), "height": g.height(),
              "style": "fill:#e0f0ff;stroke:#000000",
              ns_karabo + "class": self.__class__.__name__,
              ns_karabo + "internalKey": self.internalKey(),
              ns_karabo + "text": self.text(),
              ns_karabo + "devInstId": self.value}
        return ElementTree.Element(ns_svg + 'rect',
                                   {k: unicode(v) for k, v in d.iteritems()})


    @staticmethod
    def load(element, layout):
        internalKey = element.get("internalKey")
        Manager().newVisibleDevice(internalKey)
        Manager().selectNavigationItemByKey(internalKey)
        schema = Manager().getSchemaByInternalKey(internalKey)

        if len(schema) == 0:
            # TODO: Remove dirty hack for scientific computing again!!!
            croppedClassId = text.split("-")
            newClassId = croppedClassId[0]

            newInternalKey = internalKey
            keys = internalKey.split('+', 1)
            if len(keys) == 2:
                serverId = keys[0]
                newInternalKey = keys[0] + "+" + newClassId
                schema = Manager().getSchemaByInternalKey(newInternalKey)
                Manager().createNewDeviceClassPlugin(serverId, newClassId, text)
        customItem = GraphicsCustomItem(internalKey,
                                        layout.widget().parent().isDesignMode,
                                        text, schema)

        customItem.signalValueChanged.connect(
            Manager().onDeviceClassValueChanged)
        Manager().registerEditableComponent(customItem.deviceId, customItem)
        customItem.value = deviceId
        return customItem


####################################################################################
# This class represents a private and very specific schema reader to get the in/output#
# channels for the graphical representation                                        #
####################################################################################
class PrivateSchemaReader(object):


    def __init__(self, schema):
        super(PrivateSchemaReader, self).__init__()
        
        # Data structure for parsed information is list of Hashes
        self.__inputChannelData = []
        self.__outputChannelData = []
        
        #print ""
        #print schema
        #print ""
        
        self.__schema = schema

        keys = self.__schema.getKeys()
        for key in keys:
            self.r_readSchema(key)


    def r_readSchema(self, key):
        if self.__schema.isChoiceOfNodes(key):
            if self.__schema.hasDefaultValue(key):
                defaultValue = self.__schema.getDefaultValue(key)
            else:
                defaultValue = None
            
            if not defaultValue:
                # Default value not set - take first element
                keys = self.__schema.getKeys(key)
                if len(keys) > 0:
                    cKey = key + "." + keys[0]
                    if self.__schema.hasDisplayType(cKey):
                        defaultValue = self.__schema.getDisplayType(cKey)
            
            # Process in/output channel data
            if (self.__schema.hasDisplayType(key) and ("Input" in self.__schema.getDisplayType(key))):
                inputChannelHash = Hash()
                inputChannelHash.set("key", str(key))
                inputChannelHash.set("type", str(defaultValue))
                self.__inputChannelData.append(inputChannelHash)
            if (self.__schema.hasDisplayType(key) and ("Output" in self.__schema.getDisplayType(key))):
                outputChannelHash = Hash()
                outputChannelHash.set("key", str(key))
                outputChannelHash.set("type", str(defaultValue))
                self.__outputChannelData.append(outputChannelHash)
        else:
            keys = self.__schema.getKeys(key)
            for k in keys:
                self.r_readSchema(key + "." + k)


    def _getInputChannelData(self):
        return self.__inputChannelData
    inputChannelData = property(fget=_getInputChannelData)


    def _getOutputChannelData(self):
        return self.__outputChannelData
    outputChannelData = property(fget=_getOutputChannelData)

