import matplotlib
from matplotlib.backends.backend_qt4agg import (
    FigureCanvasQTAgg, NavigationToolbar2QT
)
from matplotlib.figure import Figure

from PyQt4.QtCore import Qt, QSize
from PyQt4.QtGui import QMessageBox

from karabo_gui import icons
from karabo_gui.messagebox import show_information

matplotlib.rcParams.update({'font.size': 8})  # default font size 10 is too big


class FigureCanvas(FigureCanvasQTAgg):

    def __init__(self, parent=None, width=4, height=3, **kwargs):
        # fig is a container which can hold any (number) of axes
        fig = Figure(figsize=(width, height), facecolor=None)
        fig.patch.set_alpha(0)  # transparent background
        super(FigureCanvas, self).__init__(fig)
        axes = self.figure.add_subplot(111)
        axes.patch.set_facecolor('white')
        self.setFocusPolicy(Qt.StrongFocus)
        self.mpl_connect('key_press_event', self.onkeypress)

    def draw(self):
        self.figure.gca().relim()
        self.figure.gca().autoscale_view()
        self.draw_idle()

    def onkeypress(self, event):
        print(event.key)
        if event.key == 'p':
            if isinstance(self.toolbar, PlotToolbar):
                self.toolbar.pan()
        elif event.key == 'z':
            if isinstance(self.toolbar, PlotToolbar):
                self.toolbar.zoom()
        elif event.key == 'home':
            if isinstance(self.toolbar, PlotToolbar):
                self.toolbar.home()
        elif event.key == 'l':
            self.toggle_axis_scale('y')
        elif event.key == 'k':
            self.toggle_axis_scale('x')

    def toggle_axis_scale(self, which):
        get_func = 'get_{}scale'.format(which)
        set_func = 'set_{}scale'.format(which)
        sc = getattr(self.figure.gca(), get_func)()
        print(sc)
        sc = 'linear' if sc == 'log' else 'log'
        getattr(self.figure.gca(), set_func)(sc)
        self.draw()

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
        super(PlotToolbar, self).__init__(canvas, parent)
        self.setIconSize(QSize(18, 18))

    def _icon(self, name):
        nm = name.split('.')[0]
        if hasattr(icons, nm):
            return getattr(icons, nm)
        else:
            return super(PlotToolbar, self)._icon(name)


    def home(self, *args):
        super(NavigationToolbar2QT, self).home(*args)
        self.canvas.figure.gca().set_autoscale_on(True)

    def helpcall(self):
        show_information("""\
home (home): reset view, start auto zoom
pan/zoom (p) mode: mouse left key to pan, right key to zoom
zoom rect (z) mode: mouse left key to zoom in, right key to zoom out
l: toggle log scale of y axis
k: toggle log scale of x axis
more to come!""",
        title="Keyboard/mouse shortcuts:")
