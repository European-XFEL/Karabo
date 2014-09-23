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

from PyQt4.QtCore import QPoint, QRectF, Qt
from PyQt4.QtGui import QColor, QFont, QFontMetrics, QFontMetricsF, QPainter, QWidget


class Item(QWidget, Loadable):

    def __init__(self, parent):
        super(Item, self).__init__(parent)
        
        self.font = QFont()
        self.displayText = ""
        
        self.inputChannels = []
        self.outputChannels = []


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
            print "input", input.current
        
        #painter.setBrush(QBrush(Qt.white))
        #painter.drawLine(self.inputPos, QPointF(40, 0))
        #if self.__connectionType == "NetworkInput-Hash":
        #    painter.drawEllipse(self.inputPos, 5, 5)


    def paintOutputChannels(self, painter):
        for output in self.outputChannels:
            print "output", output.current


    def outlineRect(self):
        padding = 5
        metrics = QFontMetricsF(self.font)
        rect = metrics.boundingRect(self.displayText)
        rect.adjust(-padding, -padding, padding, padding)
        rect.translate(-rect.center())
        return rect


    def _roundness(self, size):
        diameter = 6
        return 100 * diameter / int(size)


class WorkflowItem(Item):

    def __init__(self, device, parent):
        super(WorkflowItem, self).__init__(parent)
        
        self.device = device
        self.displayText = device.id
        
        self.descriptor = None


    def paintEvent(self, event):
        descr = self.device.descriptor
        if descr is not None and self.descriptor is None:
            self.descriptor = descr
            
            # Check for all in/output channels
            for k in descr.dict.keys():
                box = getattr(self.device.boxvalue, k, None)
                if box is None:
                    continue
                
                displayType = box.descriptor.displayType
                if displayType is None:
                    continue
                
                if displayType == "Input":
                    self.inputChannels.append(box)
                elif displayType == "Output":
                    self.outputChannels.append(box)
        
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
        
        self.device = deviceGroup
        self.displayText = deviceGroup.name


    def paintEvent(self, event):
        Item.paintEvent(self, event)


    def save(self, ele):
        ele.set(ns_karabo + "class", "WorkflowGroupItem")
        ele.set(ns_karabo + "text", self.displayText)
        ele.set(ns_karabo + "font", self.font.toString())


    @staticmethod
    def load(elem, layout):
        proxy = ProxyWidget(layout.parentWidget())
        displayText = elem.get(ns_karabo + "text")
        # TODO: get existing device group via unique id
        deviceGroup = DeviceGroup(displayText)
        item = WorkflowGroupItem(deviceGroup, proxy)
        proxy.setWidget(item)
        layout.loadPosition(elem, proxy)
        ss = [ ]
        ss.append('qproperty-font: "{}";'.format(elem.get(ns_karabo + "font")))
        item.setStyleSheet("".join(ss))
        
        return proxy

