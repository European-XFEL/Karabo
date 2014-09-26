#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 4, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["WorkflowItem", "WorkflowGroupItem"]

from const import ns_karabo
from layouts import ProxyWidget
import manager
from registry import Loadable

from PyQt4.QtCore import QPoint, QPointF, QRect, QRectF, QSize, Qt
from PyQt4.QtGui import (QBrush, QColor, QFont, QFontMetrics, QFontMetricsF,
                         QPainter, QPolygon, QWidget)


class Item(QWidget, Loadable):

    WIDTH = 5
    CHANNEL_LENGTH = 45
    

    def __init__(self, parent):
        super(Item, self).__init__(parent)
        
        self.font = QFont()
        self.displayText = ""
        
        self.inputChannels = []
        self.outputChannels = []
        
        self.descriptor = None


    def paintEvent(self, event):
        painter = QPainter()
        painter.begin(self)
        painter.setRenderHint(QPainter.Antialiasing)
        
        fm = QFontMetrics(painter.font())
        textWidth = fm.width(self.displayText)
        
        w = self.width()
        h = self.height()

        painter.translate(QPoint(w/2, h/2))
        rect = self.outlineRect()
        painter.setBrush(QColor(224,240,255)) # light blue
        painter.drawRoundRect(rect, self._roundness(rect.width()), self._roundness(rect.height()))
        painter.drawText(rect, Qt.AlignCenter, self.displayText)
        
        self.paintInputChannels(painter)
        self.paintOutputChannels(painter)
        
        #pen = painter.pen()
        #if self.isSelected():
        #    pen.setStyle(Qt.DotLine)
        #    pen.setWidth(2)
        
        #painter.setFont(self.font)
        #painter.setPen(pen)
        #painter.setBrush(QColor(224,240,255)) # light blue
        #rect = self._outlineRect()
        #print "rect", rect
        #painter.drawRoundRect(rect, self._roundness(rect.width()), self._roundness(rect.height()))
        #painter.setPen(self.textColor)
        #painter.drawText(rect, Qt.AlignCenter, self.displayText)
        painter.end()


    def paintInputChannels(self, painter):
        for input in self.inputChannels:
            # This only works for one inputChannel - height needs to be adjusted
            rect = self.outlineRect()
            startPoint = QPoint(-rect.width()/2, 0)
            endPoint = QPoint(startPoint.x() - Item.CHANNEL_LENGTH, 0)

            painter.setBrush(QBrush(Qt.white))
            painter.drawLine(startPoint, endPoint)
            
            if input.current == "BinaryFile":
                self._drawInputShape(painter, endPoint)
            elif input.current == "Hdf5File":
                self._drawRectShape(painter, QPoint(endPoint.x() - Item.WIDTH, endPoint.y()))
            elif input.current == "Network":
                self._drawCircleShape(painter, endPoint)
            elif input.current == "TextFile":
                self._drawDiamondShape(painter, QPoint(endPoint.x() + Item.WIDTH, endPoint.y()))
            else:
                self._drawCircleShape(painter, endPoint)


    def paintOutputChannels(self, painter):
        for output in self.outputChannels:
            # This only works for one inputChannel - height needs to be adjusted
            rect = self.outlineRect()
            startPoint = QPoint(rect.width()/2, 0)
            endPoint = QPoint(startPoint.x() + Item.CHANNEL_LENGTH, 0)

            painter.setBrush(QBrush(Qt.white))
            painter.drawLine(startPoint, endPoint)
            
            if output.current == "BinaryFile":
                self._drawOutputShape(painter, endPoint)
            elif output.current == "Hdf5File":
                self._drawRectShape(painter, QPoint(endPoint.x() - Item.WIDTH, endPoint.y()))
            elif output.current == "Network":
                self._drawCircleShape(painter, endPoint)
            elif output.current == "TextFile":
                self._drawDiamondShape(painter, QPoint(endPoint.x() + Item.WIDTH, endPoint.y()))
            else:
                self._drawCircleShape(painter, endPoint)


    def _drawCircleShape(self, painter, point):
        painter.drawEllipse(point, Item.WIDTH, Item.WIDTH)


    def _drawRectShape(self, painter, point):
        painter.drawRect(point.x(), point.y() - Item.WIDTH, 2 * Item.WIDTH, 2 * Item.WIDTH)


    def _drawDiamondShape(self, painter, point):
        points = [point,
                  QPoint(point.x() - Item.WIDTH, point.y() - Item.WIDTH),
                  QPoint(point.x() - 2 * Item.WIDTH, point.y()),
                  QPoint(point.x() - Item.WIDTH, point.y() + Item.WIDTH)]
        
        painter.drawPolygon(QPolygon(points))


    def _drawOutputShape(self, painter, point):
        point = QPoint(point.x() - Item.WIDTH, point.y())
        points = [point,
                  QPoint(point.x() + 2 * Item.WIDTH, point.y() - Item.WIDTH),
                  QPoint(point.x() + 2 * Item.WIDTH, point.y() + Item.WIDTH)]
        
        painter.drawPolygon(QPolygon(points))


    def _drawInputShape(self, painter, point):
        point = QPoint(point.x() + Item.WIDTH, point.y())
        points = [point,
                  QPoint(point.x() - 2 * Item.WIDTH, point.y() - Item.WIDTH),
                  QPoint(point.x() - 2 * Item.WIDTH, point.y() + Item.WIDTH)]
        
        painter.drawPolygon(QPolygon(points))


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
        Calculate bounding rectangle for this item. 
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
            for k in descr.dict.keys():
                box = getattr(device.boxvalue, k, None)
                if box is None:
                    continue
                
                displayType = box.descriptor.displayType
                if displayType is None:
                    continue
                
                if displayType == "Input":
                    self.inputChannels.append(box)
                elif displayType == "Output":
                    self.outputChannels.append(box)
            
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
        self.displayText = deviceGroup.name


    def paintEvent(self, event):
        self.checkChannels(self.deviceGroup.devices[0])
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

