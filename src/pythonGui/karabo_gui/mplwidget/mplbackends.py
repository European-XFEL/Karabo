import matplotlib
from matplotlib.backends.backend_qt4agg import \
    (FigureCanvasQTAgg, NavigationToolbar2QT)
from matplotlib.figure import Figure

from PyQt4.QtCore import Qt, QSize
from PyQt4.QtGui import QMessageBox

from karabo_gui import icons

matplotlib.rcParams.update({'font.size': 8})  # default font size 10 is too big


class FigureCanvas(FigureCanvasQTAgg):

    def __init__(self, parent=None, **kwargs):
        w = kwargs.get('width', 4)
        h = kwargs.get('height', 3)
        dpi = kwargs.get('dpi', 100)
        # fig is a container which can hold any (number) of axes
        self.fig = Figure(figsize=(w, h), dpi=dpi, facecolor=None)
        self.fig.patch.set_alpha(0)  # transparent background
        self.axes = self.fig.add_subplot(111)
        self.axes.patch.set_facecolor('white')
        FigureCanvasQTAgg.__init__(self, self.fig)
        self.setFocusPolicy(Qt.StrongFocus)

    def draw(self):
        self.fig.gca().relim()
        self.fig.gca().autoscale_view()
        self.draw_idle()


class PlotToolbar(NavigationToolbar2QT):

    def __init__(self, canvas, parent=None):
        self.toolitems = (
          ('Home', 'Reset original view', 'home', 'home'),
          ('Pan', 'Pan with left mouse, zoom with right', 'move', 'pan'),
          ('Zoom', 'Zoom to rectangle', 'zoom_to_rect', 'zoom'),
          (None, None, None, None),
          ('Subplots', 'Config subplots', 'subplots', 'configure_subplots'),
          ('Save', 'Save to file', 'filesave', 'save_figure'),
          (None, None, None, None),
          ('Help', 'Look at me!', 'helpcall', 'helpcall'),
        )
        self.canvas = canvas
        NavigationToolbar2QT.__init__(self, canvas, parent)
        self.setIconSize(QSize(18, 18))

    def _icon(self, name):
        return getattr(icons, name.split('.')[0])

    def home(self, *args):
        super(NavigationToolbar2QT, self).home(*args)
        self.canvas.figure.gca().set_autoscale_on(True)

    def helpcall(self):
        msg = QMessageBox(parent=self)
        msg.setText("Keyboard/mouse shortcuts:")
        msg.setInformativeText("""\
home: reset view, start auto zoom
pan/zoom mode: mouse left key to pan, right key to zoom
zoom rect mode: mouse left key to zoom in, right key to zoom out
more to come!
        """)
        msg.setStandardButtons(QMessageBox.Ok)
        msg.exec_()
