from PyQt4.QtGui import QWidget, QVBoxLayout

from .mplbackends import FigureCanvas, PlotToolbar
from .utils import _SHORTCUTS


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

        # use mpl's key event to dispatch key shortcuts
        self._key_event_receiver = {inst.__class__.__name__: inst for inst in
                                    (self, self._canvas, self._toolbar)}
        self._canvas.mpl_connect('key_press_event', self.on_key_press)

        l = QVBoxLayout(self)
        l.addWidget(self._canvas)
        l.addWidget(self._toolbar)

    def on_key_press(self, event):
        for klass in self._key_event_receiver:
            funcname, _ = _SHORTCUTS.get((event.key, klass), (None, None))
            if funcname is not None:
                getattr(self._key_event_receiver[klass], funcname)()
                break

    def new_curve(self, x, y, **kwargs):
        l, = self._canvas.figure.gca().plot(x, y, picker=True, **kwargs)
        self.curves.append(l)

    def update_curve(self, xdata, ydata, which=-1):
        if len(self.curves):
            self.curves[which].set_data(xdata, ydata)
            self._canvas.draw()

    def axes_call(self, func, *arg, **kwargs):
        axesfunc = getattr(self._canvas.figure.gca(), func)
        axesfunc(*arg, **kwargs)
        if func in ('set_xlabel', 'set_ylabel'):
            self._canvas.figure.tight_layout()
            self._canvas.draw()
