import matplotlib
from matplotlib.backends.backend_qt4agg import (
    FigureCanvasQTAgg, NavigationToolbar2QT
)
from matplotlib.figure import Figure
from matplotlib.lines import Line2D
import numpy as np
from PyQt4.QtCore import Qt, QSize

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
        self.highlighted = None
        self.rect = None

        self.mpl_connect('key_press_event', self.onkeypress)
        self.mpl_connect('pick_event', self.onpick)

    def draw(self):
        self.figure.gca().relim()
        self.figure.gca().autoscale_view()
        self.draw_idle()

    def onkeypress(self, event):
        update = False
        if event.key == 'k':
            self.toggle_axis_scale('x')
            update = True
        elif event.key == 'l':
            self.toggle_axis_scale('y')
            update = True
        elif event.key == 'g':
            self.figure.gca().grid()
            update = True
        elif self.toolbar:
            if event.key == 'p':
                self.toolbar.pan()
            elif event.key == 'z':
                self.toolbar.zoom()
            elif event.key == 'home':
                self.toolbar.home()
                update = True
        if update:
            self.draw()

    def toggle_axis_scale(self, which):
        ax = self.figure.gca()
        sc = {'linear': 'log', 'log': 'linear'}
        if which == 'x':
            ax.set_xscale(sc[ax.get_xscale()])
        else:
            ax.set_yscale(sc[ax.get_yscale()])
        self.draw()

    def onpick(self, event):
        if event.mouseevent.button == 1:  # left click
            if isinstance(event.artist, Line2D):
                if self.highlighted != event.artist:
                    if self.highlighted is not None:
                        self.highlighted.set_markeredgewidth(0)
                    self.highlighted = event.artist
                    if self.highlighted.get_marker() == 'None':
                        self.highlighted.set_marker(".")
                    self.highlighted.set_markeredgecolor('yellow')
                    self.highlighted.set_markeredgewidth(2)
                else:
                    self.highlighted.set_markeredgewidth(0)
                    self.highlighted = None
            event.canvas.draw()


class PlotToolbar(NavigationToolbar2QT):
    toolitems = (
      ('Home', 'Reset original view', 'home', 'home'),
      ('Pan', 'Pan with left mouse, zoom with right', 'move', 'pan'),
      ('Zoom', 'Zoom to rectangle', 'zoom_to_rect', 'zoom'),
      (None, None, None, None),
      ('Subplots', 'Config subplots', 'subplots', 'configure_subplots'),
      ('Save', 'Save to file', 'filesave', 'save_figure'),
      (None, None, None, None),
      ('Help', 'Plot widget help', 'helpcall', 'helpcall'),
    )

    def __init__(self, canvas, parent=None):
        self.canvas = canvas
        super(PlotToolbar, self).__init__(canvas, parent)
        self.setIconSize(QSize(18, 18))

    def _icon(self, name):
        nm = name.split('.')[0]
        if hasattr(icons, nm):
            return getattr(icons, nm)
        else:
            return super(PlotToolbar, self)._icon(name)

    def helpcall(self):
        show_information("""\
        g: toggle axis grid
        home (home): reset view, start auto zoom
        k: toggle log scale of x axis
        l: toggle log scale of y axis
        pan/zoom (p) mode: mouse left key to pan, right key to zoom
        zoom rect (z) mode: mouse left key to zoom in, right key to zoom out
        more to come!""", title="Keyboard/mouse shortcuts:")

    def home(self, *args):
        self.canvas.figure.gca().set_autoscale_on(True)
        self.canvas.draw()
