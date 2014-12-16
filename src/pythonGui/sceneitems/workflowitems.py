#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 4, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["WorkflowItem", "WorkflowGroupItem", "WorkflowChannel", "WorkflowConnection"]

from const import ns_karabo
from layouts import ProxyWidget
import manager
from registry import Loadable
from schema import Dummy
import scene

from PyQt4.QtCore import (pyqtSignal, QPoint, QPointF, QRect, QSize, Qt)
from PyQt4.QtGui import (QBrush, QColor, QFont, QFontMetricsF,
                         QPainter, QPainterPath, QPolygonF, QWidget)

import math


class Item(QWidget, Loadable):

    WIDTH = 5
    CHANNEL_LENGTH = 45
    INPUT = "Input"
    OUTPUT = "Output"
    

    def __init__(self, parent):
        super(Item, self).__init__(parent)
        
        self.font = QFont()
        self.displayText = ""
        
        self.input_channels = [] # list of WorkflowChannels of type input
        self.output_channels = [] # list of WorkflowChannels of type output
        
        self.descriptor = None
        
        # Position of ProxyWidget for later transformation when drawing connections
        self.proxyPos = None


    def getDeviceIds(self):
        """
        A list of all related deviceIds is returned.
        """
        raise NotImplementedError("Item.getDeviceIds")


    def getObject(self):
        """
        The object to the related device (group) is returned.
        """
        raise NotImplementedError("Item.getObject")


    def getDevice(self):
        """
        The object to the related device (group) is returned.
        """
        raise NotImplementedError("Item.getDevice")


    def mousePressEvent(self, proxy, event):
        self.proxyPos = proxy.pos()
        localPos = proxy.mapFromParent(event.pos())
        
        for input in self.input_channels:
            channelHit = input.hit(localPos)
            if channelHit:
                return input
        
        for output in self.output_channels:
            channelHit = output.hit(localPos)
            if channelHit:
                return output

        return None


    def paintEvent(self, event):
        self.checkChannels(self.getObject())

        painter = QPainter()
        painter.begin(self)
        painter.setRenderHint(QPainter.Antialiasing)
        
        #fm = QFontMetrics(painter.font())
        #textWidth = fm.width(self.displayText)

        painter.translate(QPoint(self.width()/2, self.height()/2))
        rect = self.outlineRect()
        painter.setBrush(QColor(224,240,255)) # light blue
        painter.drawRoundRect(rect, self._roundness(rect.width()), self._roundness(rect.height()))
        painter.drawText(rect, Qt.AlignCenter, self.displayText)

        self.paintInputChannels(painter)
        self.paintOutputChannels(painter)

        painter.end()


    def paintInputChannels(self, painter):
        rect = self.outlineRect()
        nbChannels = len(self.input_channels)
        yDelta = rect.height() / (nbChannels+1)
        for index, input in enumerate(self.input_channels):
            y = yDelta * index
            if nbChannels > 1:
                y -= (yDelta/2)

            painter.save()
            start_point = QPoint(-rect.width()/2, y)
            painter.translate(start_point)
            input.draw(painter)
            input.transform = painter.transform()
            painter.restore()


    def paintOutputChannels(self, painter):
        rect = self.outlineRect()
        nbChannels = len(self.output_channels)
        yDelta = rect.height() / (nbChannels+1)
        for index, output in enumerate(self.output_channels):
            y = yDelta * index
            if nbChannels > 1:
                y -= (yDelta/2)

            painter.save()
            start_point = QPoint(rect.width()/2, y)
            painter.translate(start_point)
            output.draw(painter)
            output.transform = painter.transform()
            painter.restore()


    def outlineRect(self):
        padding = 2 * Item.WIDTH
        metrics = QFontMetricsF(self.font)
        rect = metrics.boundingRect(self.displayText)
        rect.adjust(-padding, -padding, padding, padding)
        rect.translate(-rect.center())
        return rect


    def _roundness(self, size):
        diameter = 6
        return 100 * diameter / int(size)


    def boundingRect(self):
        """
        Calculate bounding rectangle for this item including channels. 
        """
        rect = self.outlineRect()
        
        addWidth = 0
        if self.input_channels:
            addWidth += 4*Item.WIDTH + 2*Item.CHANNEL_LENGTH
        elif self.output_channels:
            addWidth += 4*Item.WIDTH + 2*Item.CHANNEL_LENGTH
        
        rect.setWidth(rect.width() + addWidth)
        rect.setHeight(rect.height() + 2*Item.WIDTH)
        
        self.setMinimumWidth(rect.width())
        self.setMinimumHeight(rect.height())
        
        return rect


    def checkChannels(self, device):
        descr = device.descriptor
        if descr is not None and self.descriptor is None:
            self.descriptor = descr
            
            # Check for all in/output channels
            for k in list(descr.dict.keys()):
                box = getattr(device.boxvalue, k, None)
                if box is None:
                    continue
                
                displayType = box.descriptor.displayType
                if displayType is None:
                    continue
                
                if displayType == Item.INPUT:
                    self.input_channels.append(WorkflowChannel(displayType, box, self))
                elif displayType == Item.OUTPUT:
                    self.output_channels.append(WorkflowChannel(displayType, box, self))
            
            # Update geometry of proxy to new channels
            rect = self.boundingRect()
            pos = self.parent().fixed_geometry.topLeft()
            self.parent().fixed_geometry = QRect(pos, QSize(rect.width(), rect.height()))


    def updateConnectionsNeeded(self, transPos):
        for input in self.input_channels:
            input.updateConnectionsNeeded(transPos)
        
        for output in self.output_channels:
            output.updateConnectionsNeeded(transPos)


class WorkflowItem(Item):

    def __init__(self, device, parent):
        super(WorkflowItem, self).__init__(parent)
        
        self.device = device
        self.displayText = device.id
        

    def getDeviceIds(self):
        """
        A list of all related deviceIds is returned.
        """
        return [self.device.id]


    def getObject(self):
        """
        The object to the related device (online/offline) is returned.
        """
        if self.device.isOnline():
            return manager.getDevice(self.device.id)
        
        return self.device


    def getDevice(self):
        """
        The object to the device is returned.
        """
        return self.device


    def save(self, ele):
        ele.set(ns_karabo + "class", "WorkflowItem")
        ele.set(ns_karabo + "text", self.displayText)
        ele.set(ns_karabo + "font", self.font.toString())


    @staticmethod
    def load(elem, layout):
        # Get scene this item belongs to
        parent = layout.parent()
        while not isinstance(parent, scene.Scene):
            parent = parent.parent()
        
        project = parent.project
        
        deviceId = elem.get(ns_karabo + "text")
        
        # Get related device from project...
        device = project.getDevice(deviceId)
        
        # In case there is a device in the scene but not in the project panel
        if device is None:
            return None
        
        # Trigger selection to get descriptor
        project.signalSelectObject.emit(device)

        proxy = ProxyWidget(layout.parentWidget())
        item = WorkflowItem(device, proxy)
        proxy.setWidget(item)
        layout.loadPosition(elem, proxy)
        ss = [ ]
        ss.append('qproperty-font: "{}";'.format(elem.get(ns_karabo + "font")))
        item.setStyleSheet("".join(ss))
        
        # If device changes, make sure it gets notified
        device.addVisible()
        
        return proxy


class WorkflowGroupItem(Item):

    def __init__(self, deviceGroup, parent):
        super(WorkflowGroupItem, self).__init__(parent)
        
        self.deviceGroup = deviceGroup
        self.displayText = deviceGroup.id
        

    def getDeviceIds(self):
        """
        A list of all related deviceIds is returned.
        """
        deviceIds = []
        for device in self.deviceGroup.devices:
            deviceIds.append(device.id)
        return deviceIds


    def getObject(self):
        """
        The object to the related device group (online/offline) is returned.
        """
        if self.deviceGroup.isOnline() and self.deviceGroup.instance is not None:
            return self.deviceGroup.instance
        
        return self.deviceGroup


    def getDevice(self):
        """
        The object to the device group is returned.
        """
        return self.deviceGroup


    def save(self, ele):
        ele.set(ns_karabo + "class", "WorkflowGroupItem")
        ele.set(ns_karabo + "text", self.displayText)
        ele.set(ns_karabo + "font", self.font.toString())


    @staticmethod
    def load(elem, layout):
        # Get scene this item belongs to
        parent = layout.parent()
        while not isinstance(parent, scene.Scene):
            parent = parent.parent()
        
        project = parent.project
        
        id = elem.get(ns_karabo + "text")

        # Get related device from project...
        deviceGroup = project.getDevice(id)

        # In case there is a device in the scene but not in the project panel
        if deviceGroup is None:
            return None
        
        # Trigger selection to get descriptor
        project.signalSelectObject.emit(deviceGroup)
        
        proxy = ProxyWidget(layout.parentWidget())
        item = WorkflowGroupItem(deviceGroup, proxy)
        proxy.setWidget(item)
        layout.loadPosition(elem, proxy)
        ss = [ ]
        ss.append('qproperty-font: "{}";'.format(elem.get(ns_karabo + "font")))
        item.setStyleSheet("".join(ss))
        
        # If devices change, make sure they get notified
        deviceGroup.addVisible()
        
        return proxy


class WorkflowChannel(QWidget):
    signalUpdateConnections = pyqtSignal(object)
    
    BINARY_FILE = "BinaryFile"
    HDF5_FILE = "Hdf5File"
    NETWORK = "Network"
    TEXT_FILE = "TextFile"

    def __init__(self, channel_type, box, parent):
        super(WorkflowChannel, self).__init__(parent)
        
        assert channel_type in (Item.INPUT, Item.OUTPUT)
        self.channel_type = channel_type
        self.box = box
        self.box.signalUpdateComponent.connect(parent.update)
        
        self.painterPath = None
        
        self.start_pos = QPoint(0, 0)
        if self.channel_type == Item.INPUT:
            self.end_pos = QPoint(self.start_pos.x() - Item.CHANNEL_LENGTH, self.start_pos.y())
        elif self.channel_type == Item.OUTPUT:
            self.end_pos = QPoint(self.start_pos.x() + Item.CHANNEL_LENGTH, self.start_pos.y())

        # Matrix of painter to draw this channel
        self.transform = None


    def getDeviceIds(self):
        return self.parent().getDeviceIds()


    def draw(self, painter):
        painter.setBrush(QBrush(Qt.white))

        painter.drawLine(self.start_pos, self.end_pos)
        
        self.painterPath = QPainterPath()
        if self.box.current == WorkflowChannel.BINARY_FILE:
            self._drawTriangleShape(self.painterPath, self.end_pos)
        elif self.box.current == WorkflowChannel.HDF5_FILE:
            self._drawRectShape(self.painterPath, QPoint(self.end_pos.x() - Item.WIDTH, self.end_pos.y()))
        elif self.box.current == WorkflowChannel.NETWORK:
            self._drawCircleShape(self.painterPath, self.end_pos)
        elif self.box.current == WorkflowChannel.TEXT_FILE:
            self._drawDiamondShape(self.painterPath, QPoint(self.end_pos.x() + Item.WIDTH, self.end_pos.y()))
        else:
            self._drawTriangleShape(self.painterPath, self.end_pos)
        
        painter.drawPath(self.painterPath)
        
    
    def updateConnectionsNeeded(self, transPos):
        self.signalUpdateConnections.emit(transPos)


    def _drawCircleShape(self, painterPath, point):
        painterPath.addEllipse(point, Item.WIDTH, Item.WIDTH)


    def _drawRectShape(self, painterPath, point):
        painterPath.addRect(point.x(), point.y() - Item.WIDTH, 2 * Item.WIDTH, 2 * Item.WIDTH)


    def _drawDiamondShape(self, painterPath, point):
        points = [QPointF(point),
                  QPointF(point.x() - Item.WIDTH, point.y() - Item.WIDTH),
                  QPointF(point.x() - 2 * Item.WIDTH, point.y()),
                  QPointF(point.x() - Item.WIDTH, point.y() + Item.WIDTH)]
        
        painterPath.addPolygon(QPolygonF(points))


    def _drawTriangleShape(self, painterPath, point):
        if self.channel_type == Item.INPUT:
            self._drawInputShape(painterPath, point)
        elif self.channel_type == Item.OUTPUT:
            self._drawOutputShape(painterPath, point)


    def _drawInputShape(self, painterPath, point):
        point = QPointF(point.x() + Item.WIDTH, point.y())
        points = [point,
                  QPointF(point.x() - 2 * Item.WIDTH, point.y() - Item.WIDTH),
                  QPointF(point.x() - 2 * Item.WIDTH, point.y() + Item.WIDTH)]
        
        painterPath.addPolygon(QPolygonF(points))


    def _drawOutputShape(self, painterPath, point):
        point = QPointF(point.x() - Item.WIDTH, point.y())
        points = [point,
                  QPointF(point.x() + 2 * Item.WIDTH, point.y() - Item.WIDTH),
                  QPointF(point.x() + 2 * Item.WIDTH, point.y() + Item.WIDTH)]
        
        painterPath.addPolygon(QPolygonF(points))


    def boundingRect(self):
        if self.painterPath is None:
            return QRect()

        pathRect = self.painterPath.boundingRect()
        topLeft = QPoint(self.start_pos.x(), pathRect.y())

        width = self.end_pos.x() - self.start_pos.x()
        
        if self.channel_type == Item.INPUT:
            delta = -Item.WIDTH
        elif self.channel_type == Item.OUTPUT:
            delta = Item.WIDTH
        
        rect = QRect(topLeft, QSize(width + delta, pathRect.height()))
        return rect


    def hit(self, pos):
        """
        pos - mapped local position.
        return, whether this channel was hit or not.
        """
        rect = self.boundingRect()
        rect = self.transform.mapRect(rect)
        return rect.contains(pos)


    def allowConnection(self):
        return self.box.current == WorkflowChannel.NETWORK


    def mappedPos(self):
        point = self.transform.map(self.end_pos)
        point += self.parent().proxyPos
        return point


class WorkflowConnection(QWidget, Loadable):


    def __init__(self, parent, start_channel=None):
        super(WorkflowConnection, self).__init__(parent)
        
        # Describe the in/output channels this connection belongs to
        self.start_channel = start_channel
        self.end_channel = None
        
        if self.start_channel is None:
            self.start_channel_type = ""
            self.start_pos = QPoint()
        else:
            # Save the channel type - in the end the only channel parameter
            # necessary to draw this connection
            self.start_channel_type = self.start_channel.channel_type
            self.start_pos = self.start_channel.mappedPos()
        
        self.end_pos = self.start_pos
        self.curve = None
        
        self.proxy = None
        self.actual_start_pos = None
        #self.actual_end = None


    def mouseMoveEvent(self, event):
        self.end_pos = event.pos()
        self.update()


    def mouseReleaseEvent(self, parent, end_channel):
        if self.curve is None or self.start_channel is end_channel:
            parent.update()
            return
        
        self.end_channel = end_channel
        # Overwrite end position with exact channel position
        self.end_pos = self.end_channel.mappedPos()
        
        if self.start_pos.x() > self.end_pos.x():
            tmp = self.start_pos
            self.start_pos = self.end_pos
            self.end_pos = tmp
            
            tmp_channel = self.start_channel
            self.start_channel = self.end_channel
            self.end_channel = tmp_channel
            
            self.start_channel_type = self.start_channel.channel_type
       
        sx = self.start_pos.x()
        sy = self.start_pos.y()
        
        ex = self.end_pos.x()
        ey = self.end_pos.y()
        
        if sy > ey:
            self.start_pos.setX(0)
            self.start_pos.setY(sy - ey)

            self.end_pos.setX(ex - sx)
            self.end_pos.setY(0)
        else:
            self.start_pos.setX(0)
            self.start_pos.setY(0)

            self.end_pos.setX(ex - sx)
            self.end_pos.setY(ey - sy)
        
        self.proxy = ProxyWidget(parent.inner)
        self.proxy.setWidget(self)
        self._updateProxyGeometry()
        self.proxy.show()
        parent.ilayout.add_item(self.proxy)
        parent.setModified()
        
        # Reconfigure
        self.reconfigureInputChannel()
        
        self.start_channel.signalUpdateConnections.connect(self.onStartChannelChanged)
        self.end_channel.signalUpdateConnections.connect(self.onEndChannelChanged)


    def _updateProxyGeometry(self):
        rect = self.curve.boundingRect()
        if not hasattr(self.proxy, "fixed_geometry"):
            self.actual_start_pos = QPoint(rect.x(), rect.y())
        
        #print("##", self.actual_start_pos, rect.width(), rect.height())
        self.proxy.fixed_geometry = QRect(self.actual_start_pos.x(),
                                          self.actual_start_pos.y(),
                                          rect.width(), rect.height())


    def onStartChannelChanged(self, transPos):
        self.start_pos = self.start_pos + transPos
        self.actual_start_pos = self.actual_start_pos + transPos
        self.update()
        #self._updateProxyGeometry()


    def onEndChannelChanged(self, transPos):
        self.end_pos = self.end_pos + transPos
        self.update()
        #self._updateProxyGeometry()


    def draw(self, painter):
        #painter.setPen(self.pen)

        length = math.sqrt(self.curveWidth()**2 + self.curveHeight()**2)
        delta = length/3
        
        if self.start_channel_type == Item.OUTPUT:
            c1 = QPoint(self.start_pos.x() + delta, self.start_pos.y())
            c2 = QPoint(self.end_pos.x() - delta, self.end_pos.y())
        elif self.start_channel_type == Item.INPUT:
            c1 = QPoint(self.start_pos.x() - delta, self.start_pos.y())
            c2 = QPoint(self.end_pos.x() + delta, self.end_pos.y())
        
        self.curve = QPainterPath(self.start_pos)
        self.curve.cubicTo(c1, c2, self.end_pos)
        painter.drawPath(self.curve)
        #print()
        #print("DRAW", self.curve.boundingRect())
        #print("start, end", self.start_pos, self.end_pos)
        #print()


    def paintEvent(self, event):
        painter = QPainter()
        painter.begin(self)
        painter.setRenderHint(QPainter.Antialiasing)
        self.draw(painter)
        painter.end()


    def curveWidth(self):
        return abs(self.end_pos.x() - self.start_pos.x())


    def curveHeight(self):
        return abs(self.end_pos.y() - self.start_pos.y())


    def reconfigureInputChannel(self):
        """
        This function is called once a connection is completed and start/end channels
        are set. Then the box of the input channel needs to be notified about
        the connection to the output channel.
        """
        # Get input channel box ("connectedOutputChannels") which is going to be
        # reconfigured
        inputBox = self.end_channel.box
        value = inputBox.value
        path = inputBox.path + (inputBox.current, 'connectedOutputChannels',)
        inputChannelBox = inputBox.configuration.getBox(path)
        value = inputChannelBox.value
        
        # Get data from output channel
        outputBox = self.start_channel.box
        # Get all deviceIds of the outputChannel
        deviceIds = self.start_channel.getDeviceIds()
        connectedOutputChannels = []
        for id in deviceIds:
            output = "{}:{}".format(id, '.'.join(outputBox.path))
            connectedOutputChannels.append(output)
        
        if isinstance(value, Dummy):
            value = connectedOutputChannels
        else:
            value.extend(connectedOutputChannels)

        # Update box configuration
        inputChannelBox.set(value, None)


    def save(self, ele):
        ele.set(ns_karabo + "class", "WorkflowConnection")
        ele.set(ns_karabo + "start_channel_type", self.start_channel_type)
        ele.set(ns_karabo + "start_x", str(self.start_pos.x()))
        ele.set(ns_karabo + "start_y", str(self.start_pos.y()))
        ele.set(ns_karabo + "end_x", str(self.end_pos.x()))
        ele.set(ns_karabo + "end_y", str(self.end_pos.y()))


    @staticmethod
    def load(elem, layout):
        self.proxy = ProxyWidget(layout.parentWidget())
        connection = WorkflowConnection(self.proxy)
        connection.start_channel_type = elem.get(ns_karabo + "start_channel_type")
        
        start_x = int(elem.get(ns_karabo + "start_x"))
        start_y = int(elem.get(ns_karabo + "start_y"))
        connection.start_pos = QPoint(start_x, start_y)
        
        end_x = int(elem.get(ns_karabo + "end_x"))
        end_y = int(elem.get(ns_karabo + "end_y"))
        connection.end_pos = QPoint(end_x, end_y)
        
        self.proxy.setWidget(connection)
        layout.loadPosition(elem, self.proxy)
        
        return self.proxy

