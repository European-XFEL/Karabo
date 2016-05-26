import base64
import pickle
from xml.etree.ElementTree import Element

from PyQt4.Qwt5.Qwt import QwtPlot

from guiqwt.plot import CurveDialog
from guiqwt.builder import make

from karabo.middlelayer import NumpyVector

from karabo_gui.const import ns_karabo
from karabo_gui.widget import DisplayWidget
from karabo_gui.topology import getDevice


class XYVector(DisplayWidget):
    category = NumpyVector
    alias = "XY-Plot"

    def __init__(self, box, parent):
        super(XYVector, self).__init__(None)
        self.widget = CurveDialog(edit=False, toolbar=True,
                                  wintitle="XY-Plot")
        self.plot = self.widget.get_plot()
        self.plot.set_antialiasing(True)
        self.plot.setAxisAutoScale(QwtPlot.yLeft)
        self.plot.setAxisAutoScale(QwtPlot.xBottom)

        self.xbox = box
        self.curves = {}
        self.active = True

    def typeChanged(self, box):
        pos = QwtPlot.xBottom if box is self.xbox else QwtPlot.yLeft
        self.plot.setAxisTitle(pos, box.axisLabel())

    def addBox(self, box):
        curve = make.curve([], [], box.axisLabel(), "r")
        self.curves[box] = curve
        self.plot.add_item(curve)
        return True

    @property
    def boxes(self):
        return [self.xbox] + list(self.curves.keys())

    def valueChanged(self, box, value, timestamp=None):
        if not self.xbox.hasValue():
            return
        if box is self.xbox:
            for b, c in self.curves.items():
                if b.hasValue() and len(value) == len(b.value):
                    c.set_data(value, b.value)
        elif len(self.xbox.value) == len(value):
            self.curves[box].set_data(self.xbox.value, value)
        self.plot.replot()

    def save(self, e):
        for box, curve in self.curves.items():
            ee = Element(ns_karabo + "box")
            ee.set("device", box.configuration.id)
            ee.set("path", ".".join(box.path))
            ee.text = base64.b64encode(pickle.dumps(curve)).decode("ascii")
            e.append(ee)

    def load(self, e):
        for ee in e:
            box = getDevice(ee.get("device")).getBox(ee.get("path").split("."))
            curve = self.curves[box]
            self.plot.del_item(curve)
            curve = pickle.loads(base64.b64decode(ee.text))
            self.plot.add_item(curve)
            self.curves[box] = curve
