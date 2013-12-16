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
from layoutcomponents.graphicscustomitem import GraphicsCustomItem
from layoutcomponents.graphicsproxywidget import GraphicsProxyWidget
#from layoutcomponents.graphicsproxywidgetcontainer import GraphicsProxyWidgetContainer
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
        # List containing tuples of (internalKey, text) of item
        self.__internalKeyTextTuples = []


    def getInternalKeyTextTuples(self):
        return self.__internalKeyTextTuples


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
                        #item.setPos(QPointF(posX, posY))
                        item.setZValue(posZ)
                        self._processGraphicsItem(item)
                    elif type == "Link":
                        item = Link()
                        self.__view.scene().addItem(item)
                        #item.setPos(QPointF(posX, posY))
                        item.setZValue(posZ)
                        self._processGraphicsItem(item)
                    elif type == "Arrow":
                        item = Arrow()
                        self.__view.scene().addItem(item)
                        #item.setPos(QPointF(posX, posY))
                        item.setZValue(posZ)
                        self._processGraphicsItem(item)
                    elif type == "Line":
                        item = Line(self.__view.isDesignMode)
                        self.__view.scene().addItem(item)
                        #item.setPos(QPointF(posX, posY))
                        item.setZValue(posZ)
                        self._processGraphicsItem(item)
                    elif type == "Rectangle":
                        item = Rectangle(self.__view.isDesignMode)
                        self.__view.scene().addItem(item)
                        #item.setPos(QPointF(posX, posY))
                        item.setZValue(posZ)
                        self._processGraphicsItem(item)
                    elif type == "GraphicsProxyWidgetContainer":
                        self._processGraphicsProxyWidgetContainer(posX, posY, posZ)
                    elif type == "GraphicsProxyWidget":
                        item = self._processGraphicsProxyWidget(posX, posY, posZ)
                        item.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable)
                        self.__view.scene().addItem(item)
                    elif type == "GraphicsCustomItem":
                        item = self._processGraphicsCustomItem()
                        self.__view.scene().addItem(item)
                        #item.setPos(QPointF(posX, posY))
                        item.setZValue(posZ)
            elif tokenType == QXmlStreamReader.EndElement and tagName == "GraphicsItemList":
                break


    def _processItemAttributes(self):
        type = self.attributes().value("type").toString()
        
        posX = float(self.attributes().value("posX"))
        posY = float(self.attributes().value("posY"))
        posZ = float(self.attributes().value("posZ"))
        
        return [type, posX, posY, posZ]


    def _processGraphicsItem(self, item):
        if item is None:
            return
        
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()
            if tokenType == QXmlStreamReader.StartElement:
                if isinstance(item, Text):
                    if tagName == "sceneTransformation":
                        transform = self._processSceneTransformation()
                        # Set transformation matrix for this item
                        item.setTransform(transform)
                    elif tagName == "text":
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
                    if tagName == "sceneTransformation":
                        transform = self._processSceneTransformation()
                        # Set transformation matrix for this item
                        item.setTransform(transform)
                    elif tagName == "lineCoords":
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
                        style = int(self.text())
                    elif tagName == "color":
                        self.readNext()
                        item.setColor(QColor(self.text().toString()))
                elif isinstance(item, Rectangle):
                    if tagName == "sceneTransformation":
                        transform = self._processSceneTransformation()
                        # Set transformation matrix for this item
                        item.setTransform(transform)
                    elif tagName == "rectCoords":
                        self.readNext()
                        rectData = self.text().toString().split(",")
                        if len(rectData) == 4:
                            rect = QRectF(rectData[0].toFloat()[0], rectData[1].toFloat()[0], \
                                          rectData[2].toFloat()[0], rectData[3].toFloat()[0])
                            item.setRect(rect)

            elif tokenType == QXmlStreamReader.EndElement and tagName == "GraphicsItem":
                break


    def _processGraphicsProxyWidgetContainer(self, posX, posY, posZ):
        
        transform = None
        layoutOrientation = None
        
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()
            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "sceneTransformation":
                    transform = self._processSceneTransformation()
                elif tagName == "layoutOrientation":
                    self.readNext()
                    layoutOrientation = int(self.text())
                    if layoutOrientation[1]:
                        layoutOrientation = layoutOrientation[0]
                elif tagName == "GraphicsProxyItems":
                    self._processGraphicsProxyItemList(transform, layoutOrientation, posX, posY, posZ)
                    
            elif tokenType == QXmlStreamReader.EndElement and tagName == "GraphicsItem":
                break


    def _processGraphicsProxyItemList(self, transform, orientation, posX, posY, posZ):

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
        containerItem = customTuple[0]
        #offset = customTuple[1]
        
        #pos = QPoint(posX, posY)
        #scenePos = self.__view.mapToScene(pos)
        #scenePos = scenePos-offset
        #container.setPos(scenePos)
        
        #container.setPos(QPointF(posX, posY))
        if transform:
            # Set transformation matrix for this item
            containerItem.setTransform(transform)
        containerItem.setZValue(posZ)


    def _processGraphicsProxyWidget(self, posX, posY, posZ):

        transform = None
        componentType = None
        widgetFactory = None
        text = None
        classAlias = None
        internalKeys = None
        allowedStatesS = None
        commandEnabledS = None
        command = None
        commandText = None
        
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()
    
            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "sceneTransformation":
                    transform = self._processSceneTransformation()
                elif tagName == "componentType":
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
                elif tagName == "commandEnabled":
                    self.readNext()
                    commandEnabledS = self.text().toString()
                elif tagName == "allowedStates":
                    self.readNext()
                    allowedStatesS = self.text().toString()
                elif tagName == "command":
                    self.readNext()
                    command = self.text().toString()
                elif tagName == "commandText":
                    self.readNext()
                    commandText = self.text().toString()
            
            elif tokenType == QXmlStreamReader.EndElement and tagName == "GraphicsItem":
                break
        
        #print ""
        #print componentType, widgetFactory, text, classAlias, internalKeys
        #print ""
        
        internalKey = None
        isStateToDisplay = False
        if internalKeys:
            internalKey = str(internalKeys).split(',')
            
            if len(internalKey) == 1:
                internalKey = internalKey[0]

            # Does key concern state of device?
            print internalKey
            keys = str(internalKey).split('.configuration.')
            print keys
            isStateToDisplay = (keys[1] == "state")
        
        allowedStates = []
        if allowedStatesS is not None:
            allowedStates = str(allowedStatesS).split(',')
            
        if commandEnabledS is not None:
            commandEnabled = (str(commandEnabledS) == '1')
        
        proxyItem = None
        # Create GraphicsProxyWidget
        if componentType == "Label":
            # Label
            label = QLabel(text)
            label.setAttribute(Qt.WA_NoSystemBackground, True)
            proxyItem = GraphicsProxyWidget(self.__view.isDesignMode, label)
            proxyItem.setTransformOriginPoint(proxyItem.boundingRect().center())
        elif componentType == "DisplayComponent":
            displayComponent = None
            if classAlias != "Command":
                displayComponent = DisplayComponent(classAlias, key=internalKey, widgetFactory=widgetFactory)
            else:
                displayComponent = DisplayComponent(classAlias, key=internalKey, command=command, allowedStates=allowedStates, commandText=commandText, commandEnabled=commandEnabled, widgetFactory=widgetFactory)
            displayComponent.widget.setAttribute(Qt.WA_NoSystemBackground, True)
            proxyItem = GraphicsProxyWidget(self.__view.isDesignMode, displayComponent.widget, displayComponent, isStateToDisplay)
            proxyItem.setTransformOriginPoint(proxyItem.boundingRect().center())
        elif componentType == "EditableNoApplyComponent":
            editableComponent = EditableNoApplyComponent(classAlias, key=internalKey, widgetFactory=widgetFactory)
            editableComponent.widget.setAttribute(Qt.WA_NoSystemBackground, True)
            proxyItem = GraphicsProxyWidget(self.__view.isDesignMode, editableComponent.widget, editableComponent, isStateToDisplay)
            proxyItem.setTransformOriginPoint(proxyItem.boundingRect().center())
        elif componentType == "EditableApplyLaterComponent":
            editableComponent = EditableApplyLaterComponent(classAlias, key=internalKey, widgetFactory=widgetFactory)
            editableComponent.isEditableValueInit = False
            editableComponent.widget.setAttribute(Qt.WA_NoSystemBackground, True)
            proxyItem = GraphicsProxyWidget(self.__view.isDesignMode, editableComponent.widget, editableComponent, isStateToDisplay)
            proxyItem.setTransformOriginPoint(proxyItem.boundingRect().center())
        
        if internalKey:
            # Simulated NavigationItem click event to load schema
            Manager().selectNavigationItemByKey(internalKey)
            # Register as visible device
            Manager().newVisibleDevice(internalKey)
            # Refresh over network needed
            Manager().onRefreshInstance(internalKey)
        
        #proxyItem.setPos(QPointF(posX, posY))
        if transform:
            # Set transformation matrix for this item
            proxyItem.setTransform(transform)
        proxyItem.setZValue(posZ)
        
        return proxyItem


    def _processGraphicsCustomItem(self):
        transform = None
        internalKey = None
        text = None
        devInstId = None
        
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()
    
            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "sceneTransformation":
                    transform = self._processSceneTransformation()
                elif tagName == "internalKey":
                    self.readNext()
                    internalKey = self.text().toString()
                elif tagName == "text":
                    self.readNext()
                    text = self.text().toString()
                elif tagName == "devInstId":
                    self.readNext()
                    devInstId = self.text().toString()
            
            elif tokenType == QXmlStreamReader.EndElement and tagName == "GraphicsItem":
                break

        # Fill list with tuple
        self.__internalKeyTextTuples.append((internalKey, text))
        
        # Register as visible device
        Manager().newVisibleDevice(internalKey)
        # Simulated NavigationItem click event to load schema
        Manager().selectNavigationItemByKey(internalKey)
        # Get schema
        schema = Manager().getSchemaByInternalKey(internalKey)
        
        if len(schema) == 0:
            # DeviceClass configuration not found
            
            # TODO: Remove dirty hack for scientific computing again!!!
            croppedClassId = text.split("-")
            newClassId = croppedClassId[0]
            
            newInternalKey = internalKey
            keys = internalKey.split('+', 1)
            if len(keys) is 2:
                devSrvInsId = str(keys[0])
                newInternalKey = str(keys[0]) + "+" + newClassId
                # Try to get schema again with new internalKey
                schema = Manager().getSchemaByInternalKey(newInternalKey)
                # Create new device class plugin
                Manager().createNewDeviceClassPlugin(devSrvInsId, newClassId, text)
        
        customItem = GraphicsCustomItem(internalKey, self.__view.isDesignMode, text, schema)
        
        if transform:
            # Set transformation matrix for this item
            customItem.setTransform(transform)
        
        # Connect customItem signal to Manager, DEVICE_CLASS
        customItem.signalValueChanged.connect(Manager().onDeviceClassValueChanged)
        # Register for value changes of devInstId
        Manager().registerEditableComponent(customItem.deviceId, customItem)
        
        # Important: set devInstId after connecting necessary signals/slots
        customItem.value = devInstId
        
        return customItem


    # Function parses sceneTransformation-Tag and returns QTransform for this
    def _processSceneTransformation(self):
        m11 = 1.0
        m12 = 0.0
        m13 = 0.0

        m21 = 0.0
        m22 = 1.0
        m23 = 0.0
        
        m31 = 0.0
        m32 = 0.0
        m33 = 1.0
        
        while self.atEnd() == False:
            tokenType = self.readNext()
            tagName = self.name()
    
            if tokenType == QXmlStreamReader.StartElement:
                if tagName == "m11":
                    self.readNext()
                    m11 = self.text().toString().toFloat()
                elif tagName == "m12":
                    self.readNext()
                    m12 = self.text().toString().toFloat()
                elif tagName == "m13":
                    self.readNext()
                    m13 = self.text().toString().toFloat()
                if tagName == "m21":
                    self.readNext()
                    m21 = self.text().toString().toFloat()
                elif tagName == "m22":
                    self.readNext()
                    m22 = self.text().toString().toFloat()
                elif tagName == "m23":
                    self.readNext()
                    m23 = self.text().toString().toFloat()
                if tagName == "m31":
                    self.readNext()
                    m31 = self.text().toString().toFloat()
                elif tagName == "m32":
                    self.readNext()
                    m32 = self.text().toString().toFloat()
                elif tagName == "m33":
                    self.readNext()
                    m33 = self.text().toString().toFloat()
            
            elif tokenType == QXmlStreamReader.EndElement and tagName == "sceneTransformation":
                break

        return QTransform(m11[0], m12[0], m13[0],
                          m21[0], m22[0], m23[0],
                          m31[0], m32[0], m33[0])

