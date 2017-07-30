from PyQt4.QtGui import QWidget, QVBoxLayout

from .mplbackends import (FigureCanvas, PlotToolbar)

class MplWidget(QWidget):
    """This is a QWidget holds the canvas and plot tools. The canvas handles
    system interactions (draw, react to key/mouse press), plot tools
    are buttons with predefined functions
    """

    def __init__(self, parent=None, **kwargs):
        super(MplWidget, self).__init__()
        self._canvas = FigureCanvas(parent=self)
        self._toolbar = PlotToolbar(self._canvas, parent=self)
        self.curves = []

        l = QVBoxLayout(self)
        l.addWidget(self._canvas)
        l.addWidget(self._toolbar)

    def newCurve(self, x, y, **kwargs):
        l, = self._canvas.figure.gca().plot(x, y, picker=True, **kwargs)
        self.curves.append(l)

    def updateCurve(self, xdata, ydata, which=-1):
        if len(self.curves):
            self.curves[which].set_data(xdata, ydata)
            self._canvas.draw()

    def axes_call(self, func, *arg, **kwargs):
        axesfunc = getattr(self._canvas.figure.gca(), func)
        axesfunc(*arg, **kwargs)
        if func in ('set_xlabel', 'set_ylabel'):
            self._canvas.figure.tight_layout()
            self._canvas.draw()
