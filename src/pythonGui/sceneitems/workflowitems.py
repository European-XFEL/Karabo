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

from PyQt4.QtCore import (QPoint, QPointF, QRect, QSize, Qt)
from PyQt4.QtGui import (QBrush, QColor, QFont, QFontMetricsF,
                         QPainter, QPainterPath, QPolygon, QPolygonF,
                         QWidget)

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
        
        self.inputChannels = [] # list of WorkflowChannels of type input
        self.outputChannels = [] # list of WorkflowChannels of type output
        
        self.descriptor = None


    def mousePressEvent(self, proxy, event):
        print()
        
        for input in self.inputChannels:
            channelHit = input.hit(proxy, event.pos())
            print("=== input", channelHit)
            #if channelHit:
            #    return input
        
        for output in self.outputChannels:
            channelHit = output.hit(proxy, event.pos())
            print("=== output", channelHit)
            #if channelHit:
            #    return output

        return None


    def paintEvent(self, event):
        painter = QPainter()
        painter.begin(self)
        painter.setRenderHint(QPainter.Antialiasing)
        
        #fm = QFontMetrics(painter.font())
        #textWidth = fm.width(self.displayText)
        
        w = self.width()
        h = self.height()

        painter.translate(QPoint(w/2, h/2))
        rect = self.outlineRect()
        painter.setBrush(QColor(224,240,255)) # light blue
        painter.drawRoundRect(rect, self._roundness(rect.width()), self._roundness(rect.height()))
        painter.drawText(rect, Qt.AlignCenter, self.displayText)
        
        self.paintInputChannels(painter)
        self.paintOutputChannels(painter)

        painter.end()


    def paintInputChannels(self, painter):
        rect = self.outlineRect()
        nbChannels = len(self.inputChannels)
        yDelta = rect.height() / (nbChannels+1)
        for index, input in enumerate(self.inputChannels):
            y = yDelta * index
            if nbChannels > 1:
                y -= (yDelta/2)

            start_point = QPoint(-rect.width()/2, y)
            input.draw(start_point, painter)


    def paintOutputChannels(self, painter):
        rect = self.outlineRect()
        nbChannels = len(self.outputChannels)
        yDelta = rect.height() / (nbChannels+1)
        for index, output in enumerate(self.outputChannels):
            y = yDelta * index
            if nbChannels > 1:
                y -= (yDelta/2)

            start_point = QPoint(rect.width()/2, y)
            output.draw(start_point, painter)


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
        if self.inputChannels:
            addWidth += 4*Item.WIDTH + 2*Item.CHANNEL_LENGTH
        elif self.outputChannels:
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
                    self.inputChannels.append(WorkflowChannel(displayType, box, self))
                elif displayType == Item.OUTPUT:
                    self.outputChannels.append(WorkflowChannel(displayType, box, self))
            
            # Update geometry of proxy to new channels
            rect = self.boundingRect()
            pos = self.parent().fixed_geometry.topLeft()
            self.parent().fixed_geometry = QRect(pos, QSize(rect.width(), rect.height()))


class WorkflowItem(Item):

    def __init__(self, device, parent):
        super(WorkflowItem, self).__init__(parent)
        
        self.device = device
        self.displayText = device.id


    def paintEvent(self, event):
        self.checkChannels(self.device)
        Item.paintEvent(self, event)


    def save(self, ele):
        ele.set(ns_karabo + "class", "WorkflowItem")
        ele.set(ns_karabo + "text", self.displayText)
        ele.set(ns_karabo + "font", self.font.toString())


    @staticmethod
    def load(elem, layout):
        proxy = ProxyWidget(layout.parentWidget())
        deviceId = elem.get(ns_karabo + "text")
        item = WorkflowItem(manager.getDevice(deviceId), proxy)
        proxy.setWidget(item)
        layout.loadPosition(elem, proxy)
        ss = [ ]
        ss.append('qproperty-font: "{}";'.format(elem.get(ns_karabo + "font")))
        item.setStyleSheet("".join(ss))
        
        return proxy


class WorkflowGroupItem(Item):

    def __init__(self, deviceGroup, parent):
        super(WorkflowGroupItem, self).__init__(parent)
        
        self.deviceGroup = deviceGroup
        self.displayText = deviceGroup.id


    def paintEvent(self, event):
        if self.deviceGroup.isOnline() and self.deviceGroup.instance is not None:
            self.checkChannels(self.deviceGroup.instance)
        else:
            self.checkChannels(self.deviceGroup)
        Item.paintEvent(self, event)


    def save(self, ele):
        ele.set(ns_karabo + "class", "WorkflowGroupItem")
        ele.set(ns_karabo + "text", self.displayText)
        ele.set(ns_karabo + "font", self.font.toString())


    @staticmethod
    def load(elem, layout):
        proxy = ProxyWidget(layout.parentWidget())
        displayText = elem.get(ns_karabo + "text")
        return proxy
        # TODO: get existing device group via unique id
        deviceGroup = manager.getDeviceGroup(id) #DeviceGroup(displayText)
        item = WorkflowGroupItem(deviceGroup, proxy)
        proxy.setWidget(item)
        layout.loadPosition(elem, proxy)
        ss = [ ]
        ss.append('qproperty-font: "{}";'.format(elem.get(ns_karabo + "font")))
        item.setStyleSheet("".join(ss))
        
        return proxy


class WorkflowChannel(QWidget):


    def __init__(self, type, box, parent):
        super(WorkflowChannel, self).__init__(parent)
        
        assert type in (Item.INPUT, Item.OUTPUT)
        self.type = type
        self.box = box
        self.box.signalUpdateComponent.connect(parent.update)
        
        self.painterPath = None
        self.start_pos = None
        self.end_pos = None


    def draw(self, start_pos, painter):
        self.start_pos = start_pos
        if self.type == Item.INPUT:
            self.end_pos = QPoint(self.start_pos.x() - Item.CHANNEL_LENGTH, self.start_pos.y())
        elif self.type == Item.OUTPUT:
            self.end_pos = QPoint(self.start_pos.x() + Item.CHANNEL_LENGTH, self.start_pos.y())
        
        # Just for help - draw bounding rectangle
        painter.drawRect(self.boundingRect())
        
        painter.drawLine(self.start_pos, self.end_pos)
        
        self.painterPath = QPainterPath()
        #self.painterPath.addPolygon(QPolygonF([QPointF(self.start_pos), QPointF(self.end_pos)]))
        
        if self.box.current == "BinaryFile":
            self._drawInputShape(self.painterPath, self.end_pos)
        elif self.box.current == "Hdf5File":
            self._drawRectShape(self.painterPath, QPoint(self.end_pos.x() - Item.WIDTH, self.end_pos.y()))
        elif self.box.current == "Network":
            self._drawCircleShape(self.painterPath, self.end_pos)
        elif self.box.current == "TextFile":
            self._drawDiamondShape(self.painterPath, QPoint(self.end_pos.x() + Item.WIDTH, self.end_pos.y()))
        else:
            self._drawCircleShape(self.painterPath, self.end_pos)
        
        painter.setBrush(QBrush(Qt.white))
        painter.drawPath(self.painterPath)


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


    def _drawOutputShape(self, painterPath, point):
        point = QPoint(point.x() - Item.WIDTH, point.y())
        points = [point,
                  QPoint(point.x() + 2 * Item.WIDTH, point.y() - Item.WIDTH),
                  QPoint(point.x() + 2 * Item.WIDTH, point.y() + Item.WIDTH)]
        
        painterPath.addPolygon(QPolygon(points))


    def _drawInputShape(self, painterPath, point):
        point = QPointF(point.x() + Item.WIDTH, point.y())
        points = [point,
                  QPointF(point.x() - 2 * Item.WIDTH, point.y() - Item.WIDTH),
                  QPointF(point.x() - 2 * Item.WIDTH, point.y() + Item.WIDTH)]
        
        painterPath.addPolygon(QPolygonF(points))


    def boundingRect(self):
        if self.painterPath is None:
            return QRect()

        pathRect = self.painterPath.boundingRect()
        topLeft = QPoint(self.start_pos.x(), pathRect.y())

        width = self.end_pos.x() - self.start_pos.x()
        
        if self.type == Item.INPUT:
            delta = -Item.WIDTH
        elif self.type == Item.OUTPUT:
            delta = Item.WIDTH
        
        rect = QRect(topLeft, QSize(width + delta, pathRect.height()))
        return rect


    def hit(self, proxy, pos):
        """
        proxy - the proxy this channel belongs to
        pos - the mouse position
        """
        
        rect = self.boundingRect()
        print()
        print("++++ pos", pos.x(), pos.y())
        
        localPos = proxy.mapFromParent(pos)
        print("++++ localPos", localPos.x(), localPos.y())
        
        topLeft = rect.topLeft()
        globalTopLeft = proxy.mapToParent(topLeft)
        print(":::", topLeft, globalTopLeft)
        
        bottomRight = rect.bottomRight()
        globalBottomRight = proxy.mapToParent(bottomRight)
        print(":::", bottomRight, globalBottomRight)
        
        print()
        globalRect = QRect(globalTopLeft, globalBottomRight)
        print("localRect", rect)
        print("globalRect", globalRect, globalRect.contains(pos))
        return globalRect.contains(pos)


class WorkflowConnection(QWidget):


    def __init__(self, parent):
        super(WorkflowConnection, self).__init__(parent)
        
        self.start_pos = None
        self.end_pos = None
        self.curve = None


    def mousePressEvent(self, event):
        self.start_pos = event.pos()
        self.end_pos = self.start_pos


    def mouseMoveEvent(self, parent, event):
        self.end_pos = event.pos()
        self.update()


    def mouseReleaseEvent(self, parent, event):
        if self.curve is None:
            return
        
        if self.start_pos.x() > self.end_pos.x():
            tmp = self.start_pos
            self.start_pos = self.end_pos
            self.end_pos = tmp
       
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
        
        proxy = ProxyWidget(parent.inner)
        proxy.setWidget(self)
        rect = self.curve.boundingRect()
        proxy.fixed_geometry = QRect(int(rect.x()), int(rect.y()), 
                                     int(rect.width()), int(rect.height()))
        proxy.show()
        parent.ilayout.add_item(proxy)
        parent.setModified()


    def draw(self, painter):
        #painter.setPen(self.pen)
        
        length = math.sqrt(self.curveWidth()**2 + self.curveHeight()**2)
        delta = length/3
        
        # TODO: this is different between in/output channels
        #if self.start_pos.x() < self.end_pos.x():
        c1 = QPoint(self.start_pos.x() + delta, self.start_pos.y())
        c2 = QPoint(self.end_pos.x() - delta, self.end_pos.y())
        #else:
        #    c1 = QPoint(self.start_pos.x() - delta, self.start_pos.y())
        #    c2 = QPoint(self.end_pos.x() + delta, self.end_pos.y())
        
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

