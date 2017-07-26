
__all__ = ["XYPlot"]
import os

from karabo_gui.widget import DisplayWidget

from karabo.middlelayer import Simple

from PyQt4.Qwt5.Qwt import QwtPlot
from guiqwt.plot import CurveDialog, PlotManager
from guiqwt.builder import make
from guiqwt.tools import ToggleTool

from karabo_gui.mplwidget.mplplotwidgets import MplWidget


class Activate(ToggleTool):
    def __init__(self, manager, factory):
        super().__init__(manager, "Active")
        self.factory = factory
        self.action.setChecked(True)

    def activate_command(self, plot, checked):
        self.factory.active = checked
        if not checked:
            return
        self.factory.xvalues = []
        self.factory.yvalues = []
        self.factory.curve.set_data([], [])
        plot.replot()


class XYPlot(DisplayWidget):
    category = Simple
    alias = "XY-Plot"

    def __init__(self, box, parent):
        super(XYPlot, self).__init__(None)
        self.usempl = os.environ.get('USEMPL', 0)
        if self.usempl:
            self.widget = MplWidget()
        else:
            self.widget = CurveDialog(edit=False, toolbar=True,
                                      wintitle="XY-Plot")
            self.plot = self.widget.get_plot()
            self.plot.set_antialiasing(True)

            self.manager = PlotManager(self)
            self.manager.add_plot(self.plot)

            self.manager.register_all_curve_tools()
            self.manager.register_other_tools()
            self.manager.add_tool(Activate, self)

            self.plot.setAxisAutoScale(QwtPlot.yLeft)
            self.plot.setAxisAutoScale(QwtPlot.xBottom)
        self.xbox = box
        self.ybox = None
        self.xvalues = []
        self.yvalues = []
        self.active = True

        if box.hasValue():
            self.lastxvalue = box.value
        else:
            self.lastxvalue = None

    def typeChanged(self, box):
        if self.usempl:
            setlabel = 'set_xlabel' if box is self.xbox else 'set_ylabel'
            self.widget.axes_call(setlabel, box.axisLabel())
        else:
            pos = QwtPlot.xBottom if box is self.xbox else QwtPlot.yLeft
            self.plot.setAxisTitle(pos, box.axisLabel())

    def addBox(self, box):
        if self.ybox is None:
            self.ybox = box
            if self.usempl:
                self.widget.newCurve([], [], label="Random values")
            else:
                self.curve = make.curve([], [], 'Random values', "r")
                self.plot.add_item(self.curve)
            return True
        else:
            return False

    @property
    def boxes(self):
        if self.ybox is None:
            return [self.xbox]
        else:
            return [self.xbox, self.ybox]

    def valueChanged(self, box, value, timestamp=None):
        if not self.active:
            return
        if box is self.xbox:
            self.lastxvalue = value
        elif box is self.ybox:
            if self.lastxvalue is not None:
                self.xvalues.append(self.lastxvalue)
                self.yvalues.append(value)
                if self.usempl:
                    self.widget.updateCurve(self.xvalues, self.yvalues)
                else:
                    self.curve.set_data(self.yvalues, self.xvalues)
        else:
            raise RuntimeError("unknown box")
        if not self.usempl:
            self.plot.replot()
