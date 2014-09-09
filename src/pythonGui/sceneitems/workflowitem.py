#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 4, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["WorkflowItem"]

from registry import Loadable

from PyQt4.QtCore import QPoint, QRectF, Qt
from PyQt4.QtGui import QColor, QFont, QFontMetrics, QFontMetricsF, QPainter, QWidget


class WorkflowItem(QWidget, Loadable):

    def __init__(self, device, parent):
        super(WorkflowItem, self).__init__(parent)
        
        self.font = QFont()
        self.device = device
        self.displayText = device.id


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


    #def save(self, ele):
    #    ele.set(ns_karabo + "class", "Label")
    #    ele.set(ns_karabo + "text", self.text())
    #    ele.set(ns_karabo + "font", self.font().toString())
    #    ele.set(ns_karabo + "foreground",
    #            self.palette().color(QPalette.Foreground).name())
    #    if self.hasBackground:
    #        ele.set(ns_karabo + "background",
    #                self.palette().color(QPalette.Background).name())
    #    if self.frameShape() == QFrame.Box:
    #        ele.set(ns_karabo + 'frameWidth', "{}".format(self.lineWidth()))


    #@staticmethod
    #def load(elem, layout):
    #    proxy = ProxyWidget(layout.parentWidget())
    #    label = Label(elem.get(ns_karabo + "text"), proxy)
    #    proxy.setWidget(label)
    #    layout.loadPosition(elem, proxy)
    #    ss = [ ]
    #    ss.append('qproperty-font: "{}";'.format(elem.get(ns_karabo + "font")))
    #    ss.append("color: {};".format(
    #                elem.get(ns_karabo + "foreground", "black")))
    #    bg = elem.get(ns_karabo + 'background')
    #    if bg is not None:
    #        ss.append("background-color: {};".format(bg))
    #        label.hasBackground = True
    #    fw = elem.get(ns_karabo + "frameWidth")
    #    if fw is not None:
    #        ss.append("border: {}px;".format(fw))
    #    label.setStyleSheet("".join(ss))
    #    return proxy"""
