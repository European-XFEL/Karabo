#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 19, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a class which is a customwidget
   component for the middle panel."""

__all__ = ["GraphicsCustomItem", "PrivateXsdReader"]


from libkarabo import *

from graphicsinputchannelitem import GraphicsInputChannelItem
from graphicsoutputchannelitem import GraphicsOutputChannelItem

from layoutcomponents.nodebase import NodeBase

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class GraphicsCustomItem(NodeBase, QGraphicsObject):
    # signals
    signalValueChanged = pyqtSignal(str, object) # key, value


    def __init__(self, internalKey, isDesignMode, text, schema):
        super(GraphicsCustomItem, self).__init__(isDesignMode)
        
        self.__textFont = QFont()
        self.__text = text
        
        self.__devInsId = ""
        
        self.__compositeText = None
        self._updateCompositeText()
        
        self.__internalKey = internalKey
        
        self.__inputChannelData = [] # List contains Hashes with input data
        self.__outputChannelData = [] # List contains Hashes with output data
        
        self.__inputChannelItems = [] # List contains GraphicsInputChannelItem
        self.__outputChannelItems = [] # List contains GraphicsOutputChannelItem
        
        if schema:
            xsdParser = PrivateXsdReader(schema)
            # Get information to draw possible in/output channels
            self.__inputChannelData = xsdParser.inputChannelData
            self.__outputChannelData = xsdParser.outputChannelData
            # Update channel graphics representation
            self._updateChannelItems()

        self.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable | QGraphicsItem.ItemSendsGeometryChanges)


    def internalKey(self):
        return self.__internalKey


    def _getValue(self):
        return self.__devInsId
    def _setValue(self, value):
        if self.__devInsId == value:
            return
        
        # Prepare item for change
        self.prepareGeometryChange()
        # Prepare channel item for change
        self._prepareGeometryChangeChannelItems()
        
        self.__devInsId = value
        self._updateCompositeText()
        
        # Update item geometry
        self.update()
        # Update channel item geometry
        self._updateChannelItems()
        
        # Send changing signal to Manager (connected only for DEVICE_CLASS)
        self.signalValueChanged.emit(self.devInstIdKey, self.__devInsId)
    value = property(fget=_getValue, fset=_setValue)


    def _getDevInstIdKey(self):
        return self.__internalKey + ".devInstId"
    devInstIdKey = property(fget=_getDevInstIdKey)


    def text(self):
        return self.__text


    def inputChannelItems(self):
        return self.__inputChannelItems


    def outputChannelItems(self):
        return self.__outputChannelItems


    def boundingRect(self):
        margin = 80 # 1, TODO: consider channels as well..
        return self._outlineRect().adjusted(-margin, -margin, +margin, +margin)


    def shape(self):
        rect = self._outlineRect()
        path = QPainterPath()
        path.addRoundRect(rect, self._roundness(rect.width()), self._roundness(rect.height()))
        return path


    def paint(self, painter, option, widget):
        #pen = QPen(self.__outlineColor)
        pen = painter.pen()
        if self.isSelected():
            pen.setStyle(Qt.DotLine)
            pen.setWidth(2)
        
        painter.setFont(self.__textFont)
        painter.setPen(pen)
        #painter.setBrush(self.__backgroundColor)
        rect = self._outlineRect()
        painter.drawRoundRect(rect, self._roundness(rect.width()), self._roundness(rect.height()))
        #painter.setPen(self.__textColor)
        painter.drawText(rect, Qt.AlignCenter, self.__compositeText)


### private ###
    def _updateCompositeText(self):
        self.__compositeText = str(self.__text + "\n<" + self.__devInsId + ">")


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
                inputChannelItem = GraphicsInputChannelItem(self, hash.get("type"))
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
                outputChannelItem = GraphicsOutputChannelItem(self, hash.get("type"))
                self.__outputChannelItems.append(outputChannelItem)
            else:
                outputChannelItem = self.__outputChannelItems[i]
            outputChannelItem.setPos(QPointF(x, y))


    def _outlineRect(self):
        padding = 4
        metrics = QFontMetricsF(self.__textFont) #qApp.fontMetrics())
        rect = metrics.boundingRect(self.__compositeText)
        rect.adjust(-padding, -padding, +padding, +padding)
        rect.translate(-rect.center())
        return rect


    def _roundness(self, size):
        diameter = 6
        return 100 * diameter / int(size)


### protected ###
    def mouseDoubleClickEvent(self, event):
        additionalText = QInputDialog.getText(event.widget(), "Edit device instance", \
                                              "Enter new device instance:", QLineEdit.Normal, \
                                              self.__devInsId)
        
        if len(additionalText[0]) > 0:
            self.value = additionalText[0]

        QGraphicsObject.mouseDoubleClickEvent(self, event)


    def itemChange(self, change, value):
        if change == QGraphicsItem.ItemPositionHasChanged: # ItemPositionChange
            for inputChannel in self.__inputChannelItems:
                inputChannel.trackChannelConnection()

            for outputChannel in self.__outputChannelItems:
                outputChannel.trackChannelConnection()
        return QGraphicsItem.itemChange(self, change, value)


### slots ###
    # Triggered by DataNotifier signalUpdateComponent
    def onValueChanged(self, key, value):
        if self.devInstIdKey == key:
            self.value = value


####################################################################################
# This class represents a private and very specific XSD parser to get the in/output#
# channels for the graphical representation                                        #
####################################################################################
class PrivateXsdReader(QXmlStreamReader):


    def __init__(self, schema):
        super(PrivateXsdReader, self).__init__()
        
        # Data structure for parsed information is list of Hashes
        self._inputChannelData = []
        self._outputChannelData = []
        
        self.clear()
        self.addData(schema)

        self.readNext() # StartDocument
        self.readNext()

        if self.name() != "schema":
            print "Configurator was fed with illegal XSD"
            return

        self.processMainElementTag()


    def _getInputChannelData(self):
        return self._inputChannelData
    inputChannelData = property(fget=_getInputChannelData)


    def _getOutputChannelData(self):
        return self._outputChannelData
    outputChannelData = property(fget=_getOutputChannelData)


    def processMainElementTag(self):
    
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "element":
                    self.processSimpleElements()
            elif tokenType == QXmlStreamReader.EndElement:
                break


    def processSimpleElements(self):

        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()
            
            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "element":
                    name, type, defaultValue, minOccurs, maxOccurs = self.processSimpleElementAttributes()
                    # Following tags
                    self.processFollowingElements(name, type, defaultValue, minOccurs, maxOccurs)
            elif tokenType == QXmlStreamReader.EndElement:
                break


    def processSimpleElementAttributes(self):
        
        name = self.attributes().value("name").toString()

        if self.attributes().hasAttribute("type"):
            type = self.attributes().value("type").toString()
        else:
            type = None

        if self.attributes().hasAttribute("default"):
            defaultValue = self.attributes().value("default").toString()
        else:
            defaultValue = None

        if self.attributes().hasAttribute("minOccurs"):
            minOccurs = self.attributes().value("minOccurs").toString()
        else:
            minOccurs = None

        if self.attributes().hasAttribute("maxOccurs"):
            maxOccurs = self.attributes().value("maxOccurs").toString()
        else:
            maxOccurs = None

        return [name, type, defaultValue, minOccurs, maxOccurs]


    def processFollowingElements(self, name=None, type=None, defaultValue=None, minOccurs=None, maxOccurs=None):

        description = None
        displayedName = None
        expertLevel = None
        default = None
        unitName = None
        unitSymbol = None
        accessType = None
        displayType = None
        allowedStates = None
        
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "annotation":
                    description, displayedName, expertLevel, default, unitName, unitSymbol, accessType, displayType, allowedStates = self.processAnnotationTag()
                    # Process in/output channel data
                    if displayType == "Input":
                        inputChannel = Hash("type", str(default))
                        self._inputChannelData.append(inputChannel)
                        self.processInputChannelParameter()
                    elif displayType == "Output":
                        outputChannel = Hash("type", str(default))
                        self._outputChannelData.append(outputChannel)
                        self.processOutputChannelParameter()

                elif tagName == "simpleType":
                    restrictionBase, minInclusive, maxInclusive, enumeration = self.processSimpleTypeTag()
                elif tagName == "complexType":
                    self.processComplexTypeTag()
            
            elif tokenType == QXmlStreamReader.EndElement and tagName == "element":
                break


    def processAnnotationTag(self):

        description = None
        displayedName = None
        expertLevel = None
        default = None
        unitName = None
        unitSymbol = None
        accessType = None
        displayType = None
        allowedStates = None

        while self.atEnd() == False :
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "description":
                    self.readNext()
                    description = self.text().toString()
                elif tagName == "displayedName":
                    self.readNext()
                    displayedName = self.text().toString()
                elif tagName == "expertLevel":
                    self.readNext()
                    expertLevel = self.text().toString()
                elif tagName == "default":
                    self.readNext()
                    default = self.text().toString()
                elif tagName == "unitName":
                    self.readNext()
                    unitName = self.text().toString()
                elif tagName == "unitSymbol":
                    self.readNext()
                    unitSymbol = self.text().toString()
                elif tagName == "accessType":
                    self.readNext()
                    accessType = self.text().toString()
                elif tagName == "displayType":
                    self.readNext()
                    displayType = self.text().toString()
                elif tagName == "allowedStates":
                    self.readNext()
                    allowedStates = list(self.text().toString().split(','))
            elif tokenType == QXmlStreamReader.EndElement and tagName == "annotation":
                break

        return [description, displayedName, expertLevel, default, unitName, unitSymbol, accessType, displayType, allowedStates]


    def processSimpleTypeTag(self):

        restrictionBase = None
        minInclusive = None
        maxInclusive = None
        enumeration = []

        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "restriction":
                    restrictionBase = self.attributes().value("base").toString()
                elif tagName == "minInclusive":
                    minInclusive = self.attributes().value("value").toString()
                elif tagName == "maxInclusive":
                    maxInclusive = self.attributes().value("value").toString()
                elif tagName == "enumeration":
                    enumeration.append(self.attributes().value("value").toString())
            elif tokenType == QXmlStreamReader.EndElement and tagName == "simpleType":
                break

        return [restrictionBase, minInclusive, maxInclusive, enumeration]


    def processInputChannelParameter(self):

        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "choice":
                    self.processChannelChoiceTag()
                #elif tagName == "element":
                #    name, type, defaultValue, minOccurs, maxOccurs = self.processSimpleElementAttributes()
                    # process children
                #    self.processFollowingElements(name, type, defaultValue, minOccurs, maxOccurs)
                #elif tagName == "sequence":
                #    self.processSequenceTag()
                #elif tagName == "attribute":
                #    name = self.attributes().value("name").toString()
                #    type = self.attributes().value("type").toString()
                #    default = self.attributes().value("default").toString()
            elif tokenType == QXmlStreamReader.EndElement and tagName == "complexType":
                break


    def processOutputChannelParameter(self):

        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "choice":
                    self.processChannelChoiceTag()
                #elif tagName == "element":
                #    name, type, defaultValue, minOccurs, maxOccurs = self.processSimpleElementAttributes()
                    # process children
                #    self.processFollowingElements(name, type, defaultValue, minOccurs, maxOccurs)
                #elif tagName == "sequence":
                #    self.processSequenceTag()
                #elif tagName == "attribute":
                #    name = self.attributes().value("name").toString()
                #    type = self.attributes().value("type").toString()
                #    default = self.attributes().value("default").toString()
            elif tokenType == QXmlStreamReader.EndElement and tagName == "complexType":
                break


    def processChannelChoiceTag(self):
        while self.atEnd() == False :
            tokenType = self.readNext()
            tagName = self.name()
            
            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "element":
                    name, type, defaultValue, minOccurs, maxOccurs = self.processSimpleElementAttributes()
                    self.processChannelFollowingElements(name, type, defaultValue, minOccurs, maxOccurs)

            elif tokenType == QXmlStreamReader.EndElement and tagName == "choice":
                break


    def processChannelFollowingElements(self, name, type, defaultValue, minOccurs, maxOccurs):
        
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "simpleType":
                    restrictionBase, minInclusive, maxInclusive, enumeration = self.processSimpleTypeTag()
                elif tagName == "complexType":
                    self.processChannelComplexTypeTag()
            
            elif tokenType == QXmlStreamReader.EndElement and tagName == "element":
                break


    def processChannelComplexTypeTag(self):
        
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()
            
            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "choice":
                    self.processChoiceTag(isSequenceElement)
                elif tagName == "element":
                    name, type, defaultValue, minOccurs, maxOccurs = self.processSimpleElementAttributes()
                    # process children
                    self.processChannelFollowingElements(name, type, defaultValue, minOccurs, maxOccurs)
                elif tagName == "sequence":
                    self.processSequenceTag()
                elif tagName == "attribute":
                    name = self.attributes().value("name").toString()
                    type = self.attributes().value("type").toString()
                    default = self.attributes().value("default").toString()
            elif tokenType == QXmlStreamReader.EndElement and tagName == "complexType":
                break


    def processComplexTypeTag(self, isSequenceElement=False):

        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "choice":
                    self.processChoiceTag(isSequenceElement)
                elif tagName == "element":
                    name, type, defaultValue, minOccurs, maxOccurs = self.processSimpleElementAttributes()
                    # process children
                    self.processFollowingElements(name, type, defaultValue, minOccurs, maxOccurs)
                elif tagName == "sequence":
                    self.processSequenceTag()
                elif tagName == "attribute":
                    name = self.attributes().value("name").toString()
                    type = self.attributes().value("type").toString()
                    default = self.attributes().value("default").toString()
            elif tokenType == QXmlStreamReader.EndElement and tagName == "complexType":
                break


    def processChoiceTag(self, isSequenceElement=False):

        while self.atEnd() == False :
            tokenType = self.readNext()
            tagName = self.name()
            
            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "element":
                    name, type, defaultValue, minOccurs, maxOccurs = self.processSimpleElementAttributes()
                    self.processFollowingElements(name, type, defaultValue, minOccurs, maxOccurs)

            elif tokenType == QXmlStreamReader.EndElement and tagName == "choice":
                break


    def processSequenceTag(self):

        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()

            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "element":
                    name, type, defaultValue, minOccurs, maxOccurs = self.processSimpleElementAttributes()

                self.processFollowingElements(name, type, defaultValue, minOccurs, maxOccurs)

            elif tokenType == QXmlStreamReader.EndElement and tagName == "sequence":
                break

