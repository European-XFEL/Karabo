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
                         QPainter, QPainterPath, QPen, QPolygonF, QWidget)

import math


class Item(QWidget, Loadable):

    WIDTH = 5
    CHANNEL_LENGTH = 20
    INPUT = "Input"
    OUTPUT = "Output"
    

    def __init__(self, scene, proxy):
        super(Item, self).__init__(proxy)
        
        self.pen = QPen()
        self.font = QFont()
        self.displayText = ""
        
        self.input_channels = [] # list of WorkflowChannels of type input
        self.output_channels = [] # list of WorkflowChannels of type output
        
        self.descriptor = None
        
        # Position of ProxyWidget for later transformation when drawing connections
        self.proxyPos = proxy.pos()
        
        self.scene = scene
        self.connectionsChecked = False


    def clear(self):
        """
        All channels are cleaned.
        """
        for input in self.input_channels:
            input.clear()
        
        for output in self.output_channels:
            output.clear()


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
        painter = QPainter()
        painter.begin(self)
        painter.setRenderHint(QPainter.Antialiasing)
        painter.setPen(self.pen)
        
        #fm = QFontMetrics(painter.font())
        #textWidth = fm.width(self.displayText)

        painter.translate(QPoint(self.width()/2, self.height()/2))
        rect = self.outlineRect()
        painter.setBrush(QColor(224,240,255)) # light blue
        painter.drawRoundRect(rect, self._roundness(rect.width()), self._roundness(rect.height()))
        painter.drawText(rect, Qt.AlignCenter, self.displayText)

        self.checkChannels(self.getObject())
        self.paintInputChannels(painter)
        self.paintOutputChannels(painter)
        self.checkConnections()

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
            painter.setPen(QPen())
            input.transform = painter.transform()
            input.draw(painter)
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
            output.transform = painter.transform()
            output.draw(painter)
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
            pos = self.parent().geometry().topLeft()
            self.parent().set_geometry(QRect(pos, QSize(rect.width(), rect.height())))
            
            # Set the proxyPos for later calculation (workflow connections)
            self.proxyPos = self.parent().pos()
                

    def checkConnections(self):
        raise NotImplementedError("Item.checkConnections")


    def updateConnectionsNeeded(self, scene, transPos):
        for input in self.input_channels:
            input.updateConnectionsNeeded(scene, transPos)
        
        for output in self.output_channels:
            output.updateConnectionsNeeded(scene, transPos)


class WorkflowItem(Item):

    def __init__(self, device, scene, proxy):
        super(WorkflowItem, self).__init__(scene, proxy)
        
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
        The object to the project device is returned.
        """
        return self.device


    def checkConnections(self):
        if len(self.input_channels) < 1 or self.connectionsChecked:
            return
        
        for input in self.input_channels:
            path = input.box.path + ('connectedOutputChannels',)
            if not input.box.configuration.hasBox(path):
                return

            connectedOutputs = input.box.configuration.getBox(path).value
            if isinstance(connectedOutputs, Dummy):
                return

            for output in connectedOutputs:
                output = output.replace(":" ,".")
                start_channel = self.scene.getOutputChannelItem([output])
                if start_channel is None:
                    continue
                
                wc = WorkflowConnection(None, start_channel)
                wc.updateValues(self.scene, input)
                wc.proxy.set_geometry(QRect(wc.topLeft, QSize(wc.curveWidth(), wc.curveHeight())))
                
        self.connectionsChecked = True


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
        item = WorkflowItem(device, parent, proxy)
        proxy.setWidget(item)
        layout.loadPosition(elem, proxy)
        ss = [ ]
        ss.append('qproperty-font: "{}";'.format(elem.get(ns_karabo + "font")))
        item.setStyleSheet("".join(ss))
        
        # If device changes, make sure it gets notified
        device.addVisible()
        
        return proxy


class WorkflowGroupItem(Item):

    def __init__(self, deviceGroup, scene, proxy):
        super(WorkflowGroupItem, self).__init__(scene, proxy)
        
        self.deviceGroup = deviceGroup
        self.displayText = deviceGroup.id
        
        self.pen.setWidth(2)
        

    def getDeviceIds(self):
        """
        A list of all related deviceIds is returned.
        """
        return [device.id for device in self.deviceGroup.devices]


    def getObject(self):
        """
        The object to the related device group (online/offline) is returned.
        """
        if self.deviceGroup.isOnline() and self.deviceGroup.instance is not None:
            return self.deviceGroup.instance
        
        return self.deviceGroup


    def getDevice(self):
        """
        The object to the project device group is returned.
        """
        return self.deviceGroup


    def checkConnections(self):
        if len(self.input_channels) < 1 or self.connectionsChecked:
            return
        
        for input in self.input_channels:
            path = input.box.path + ('connectedOutputChannels',)
            if not input.box.configuration.hasBox(path):
                return

            connectedOutputs = input.box.configuration.getBox(path).value
            if isinstance(connectedOutputs, Dummy):
                return


            output = [o.replace(":" ,".") for o in connectedOutputs]
            start_channel = self.scene.getOutputChannelItem(output)
            if start_channel is None:
                continue
            
            wc = WorkflowConnection(None, start_channel)
            wc.updateValues(self.scene, input)
            wc.proxy.set_geometry(QRect(wc.topLeft, QSize(wc.curveWidth(), wc.curveHeight())))
                
        self.connectionsChecked = True


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
        item = WorkflowGroupItem(deviceGroup, parent, proxy)
        proxy.setWidget(item)
        layout.loadPosition(elem, proxy)
        ss = [ ]
        ss.append('qproperty-font: "{}";'.format(elem.get(ns_karabo + "font")))
        item.setStyleSheet("".join(ss))
        
        # If devices change, make sure they get notified
        deviceGroup.addVisible()
        
        return proxy


class WorkflowChannel(QWidget):
    signalUpdateConnections = pyqtSignal(object, object) # scene, translationPoint

    def __init__(self, channel_type, box, parent):
        super(WorkflowChannel, self).__init__(parent)
        
        assert channel_type in (Item.INPUT, Item.OUTPUT)
        self.channel_type = channel_type
        self.box = box
        
        self.painterPath = None
        
        self.start_pos = QPoint(0, 0)
        if self.channel_type == Item.INPUT:
            self.end_pos = QPoint(self.start_pos.x() - Item.CHANNEL_LENGTH, self.start_pos.y())
        elif self.channel_type == Item.OUTPUT:
            self.end_pos = QPoint(self.start_pos.x() + Item.CHANNEL_LENGTH, self.start_pos.y())

        # Matrix of painter to draw this channel
        self.transform = None
        
        self.workflow_connections = []


    def clear(self):
        """
        All associated workflow connections are removed.
        """
        i = len(self.workflow_connections)
        while self.workflow_connections:
            i -= 1
            self.removeWorkflowConnection(self.workflow_connections[i])


    def getDeviceIds(self):
        return self.parent().getDeviceIds()


    def draw(self, painter):
        painter.setBrush(QBrush(Qt.white))

        painter.drawLine(self.start_pos, self.end_pos)
        
        self.painterPath = QPainterPath()
        self._drawCircleShape(self.painterPath, self.end_pos)
        
        painter.drawPath(self.painterPath)
        
    
    def updateConnectionsNeeded(self, scene, transPos):
        self.signalUpdateConnections.emit(scene, transPos)


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


    def mappedPos(self):
        point = self.transform.map(self.end_pos)
        point += self.parent().proxyPos
        return point


    def appendWorkflowConnection(self, wc):
        """
        The given workflow connection object \wc is added to this channels'
        workflow connection list.
        """
        self.workflow_connections.append(wc)


    def removeWorkflowConnection(self, wc):
        """
        The given workflow connection object \wc is removed from this channels'
        workflow connection list.
        Furthermore the associated ProxyWidget is removed from the scene as well.
        """
        self.parent().scene.ilayout.remove_item(wc.proxy)
        
        if wc in self.workflow_connections:
            index = self.workflow_connections.index(wc)
            self.workflow_connections.pop(index)


class WorkflowConnection(QWidget):
    
    # Data distribution type
    COPY = "copy"
    SHARED = "shared"


    def __init__(self, proxy, start_channel):
        super(WorkflowConnection, self).__init__(proxy)
        
        # Describe the in/output channels this connection belongs to
        self.start_channel = start_channel
        self.end_channel = None
        
        # Save the channel type - in the end the only channel parameter
        # necessary to draw this connection
        self.start_channel_type = self.start_channel.channel_type
        self.start_pos = self.start_channel.mappedPos()
        
        self.end_pos = self.start_pos
        self.curve = QPainterPath(self.start_pos)
        
        # Global start/end positions
        self.global_start = None
        self.global_end = None
        
        self.proxy = proxy
        # Top left position of proxy in global coordinates
        self.topLeft = QPoint()
        
        self.dataDistribution = None


    def mouseMoveEvent(self, event):
        self.end_pos = event.pos()
        self.update()


    def mouseReleaseEvent(self, parent, end_channel):
        if self.curve is None or self.start_channel is end_channel:
            parent.update()
            return
        
        self.updateValues(parent, end_channel)
        
        # Reconfigure
        self.reconfigureInputChannel()
        parent.setModified()


    def _checkStartEnd(self):
        """
        
        """
        sx = self.global_start.x()
        sy = self.global_start.y()
        
        ex = self.global_end.x()
        ey = self.global_end.y()
        
        if sy > ey:
            self.start_pos.setX(0)
            self.start_pos.setY(sy - ey)

            self.end_pos.setX(ex - sx)
            self.end_pos.setY(0)

            self.topLeft.setX(sx)
            self.topLeft.setY(ey)
        else:
            if sy < ey:
                self.start_pos.setX(0)
                self.start_pos.setY(0)

                self.end_pos.setX(ex - sx)
                self.end_pos.setY(ey - sy)
            
            self.topLeft.setX(sx)
            self.topLeft.setY(sy)


    def _updateProxyGeometry(self):
        rect = self.curve.boundingRect()
        rect = QRect(self.topLeft.x(), self.topLeft.y(),
                     rect.width(), rect.height())
        self.proxy.set_geometry(rect)


    def updateValues(self, parent, end_channel):
        self.end_channel = end_channel
        # Overwrite end position with exact channel position
        self.end_pos = self.end_channel.mappedPos()
        
        if not hasattr(self.end_channel.box.value, "connectedOutputChannels"):
            self.start_pos, self.end_pos = self.end_pos, self.start_pos
            self.start_channel, self.end_channel = (
                self.end_channel, self.start_channel)

            self.start_channel_type = self.start_channel.channel_type
       
        self.global_start = QPoint(self.start_pos)
        self.global_end = QPoint(self.end_pos)
        self._checkStartEnd()
        
        if self.proxy is None:
            self.proxy = ProxyWidget(parent.inner)
            self.proxy.setWidget(self)
            self._updateProxyGeometry()
            self.proxy.show()
            parent.ilayout.add_item(self.proxy)
            self.proxy.lower()

            self.start_channel.signalUpdateConnections.connect(self.onStartChannelChanged)
            self.end_channel.signalUpdateConnections.connect(self.onEndChannelChanged)
            
            if hasattr(self.end_channel.box.value, "dataDistribution"):
                dataDistribution = self.end_channel.box.boxvalue.dataDistribution
                self.dataDistribution = dataDistribution.value
                dataDistribution.signalUpdateComponent.connect(self.onDataDistributionChanged)

            connectedOutputs = self.end_channel.box.boxvalue.connectedOutputChannels
            connectedOutputs.signalUpdateComponent.connect(self.onConnectedOutputChannelsChanged)
            
            self.end_channel.appendWorkflowConnection(self)


    def onStartChannelChanged(self, scene, transPos):
        self.global_start = self.global_start + transPos
        self._checkStartEnd()
        self._updateProxyGeometry()
        scene.ilayout.update()


    def onEndChannelChanged(self, scene, transPos):
        self.global_end = self.global_end + transPos
        self._checkStartEnd()
        self._updateProxyGeometry()
        scene.ilayout.update()


    def onDataDistributionChanged(self, _, value):
        self.dataDistribution = value
        self.update()


    def onConnectedOutputChannelsChanged(self, box, value):
        if not value:
            # Remove connection
            self.end_channel.removeWorkflowConnection(self)


    def draw(self, painter):
        length = math.sqrt(self.curveWidth()**2 + self.curveHeight()**2)
        delta = length/3
        
        if self.start_channel_type == Item.OUTPUT:
            c1 = QPoint(self.start_pos.x() + delta, self.start_pos.y())
            c2 = QPoint(self.end_pos.x() - delta, self.end_pos.y())
        elif self.start_channel_type == Item.INPUT:
            c1 = QPoint(self.start_pos.x() - delta, self.start_pos.y())
            c2 = QPoint(self.end_pos.x() + delta, self.end_pos.y())
        
        if self.dataDistribution == WorkflowConnection.SHARED:
            painter.setPen(QPen(Qt.DashLine))
        
        self.curve = QPainterPath(self.start_pos)
        self.curve.cubicTo(c1, c2, self.end_pos)
        painter.drawPath(self.curve)


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
        deviceIds = self.start_channel.getDeviceIds()

        endBox = self.end_channel.box
        path = ".".join(self.start_channel.box.path)
        if not endBox.boxvalue.connectedOutputChannels.hasValue():
            endBox.value.connectedOutputChannels = []
            
        endBox.value.connectedOutputChannels.extend(s for s in (
                            "{}:{}".format(id, path) for id in deviceIds)
                            if s not in endBox.value.connectedOutputChannels)

        endBox.boxvalue.connectedOutputChannels.update()


    def removeConnectedOutputChannel(self):
        """
        This function is called once this connection is about to be removed and
        the connected output channels of the input channel needs to be updated.
        """
        if self.start_channel is None and self.end_channel is None:
            return

        path = ".".join(self.start_channel.box.path)
        paths = {"{}:{}".format(id, path)
                 for id in self.start_channel.getDeviceIds()}
        inputChannel = self.end_channel.box.value
        inputChannel.connectedOutputChannels = [
            c for c in inputChannel.connectedOutputChannels if c not in paths]

