import matplotlib
from matplotlib.backends.backend_qt4agg import (
    FigureCanvasQTAgg, NavigationToolbar2QT
)
from matplotlib.figure import Figure
from matplotlib.lines import Line2D
from PyQt4.QtCore import Qt, QSize

from karabo_gui import icons
from karabo_gui.messagebox import show_information
from .utils import register_shortcut, _SHORTCUTS

matplotlib.rcParams.update({'font.size': 8})  # default font size 10 is too big


class FigureCanvas(FigureCanvasQTAgg):

    def __init__(self, *, parent=None, width=4, height=3, **kwargs):
        # fig is a container which can hold any (number) of axes
        fig = Figure(figsize=(width, height), facecolor=None)
        fig.patch.set_alpha(0)  # transparent background
        super(FigureCanvas, self).__init__(fig)
        self.curr_axes = fig.add_subplot(111)

        self.setFocusPolicy(Qt.StrongFocus)
        self.highlighted = None
        self.toolbar = None  # this will be set by the toolbar class

        self.mpl_connect('button_press_event', self.on_mouse_click)
        self.mpl_connect('pick_event', self.on_pick)

    def draw(self, new_data=True):
        if new_data:
            self.curr_axes.relim()  # recalculate data limit
        self.curr_axes.autoscale_view()
        super(FigureCanvas, self).draw()

    def on_mouse_click(self, event):
        if self.toolbar:
            if event.dblclick and not event.inaxes:
                self.toolbar.edit_parameters()

    def on_pick(self, event):
        if event.mouseevent.button == 1:  # left click
            picked = event.artist
            if isinstance(picked, Line2D):
                if self.highlighted is picked:
                    picked.set_markeredgewidth(0)
                    self.highlighted = None
                else:
                    if self.highlighted is not None:
                        self.highlighted.set_markeredgewidth(0)
                    if picked.get_marker() == 'None':
                        picked.set_marker(".")
                    picked.set_markeredgecolor('yellow')
                    picked.set_markeredgewidth(2)
                    self.highlighted = picked
            self.draw(new_data=False)

    def toggle_axis_scale(self, which):
        ax = self.curr_axes
        if which == 'x':
            set_scale = ax.set_xscale
            get_scale = ax.get_xscale
        else:
            set_scale = ax.set_yscale
            get_scale = ax.get_yscale
        sc = {'linear': 'log', 'log': 'linear'}
        curr_scale = get_scale()
        try:
            set_scale(sc[curr_scale])
        except ValueError as e:
            # mpl raises ValueError if use log scale with no positive value
            # in the plot, we swallow the error and set scale back to linear
            print('WARNING:', e)
            set_scale(curr_scale)
        else:
            self.draw(new_data=False)

    @register_shortcut(key='k')
    def _toggle_axis_scalex(self):
        """toggle log scale of x axis"""
        self.toggle_axis_scale('x')

    @register_shortcut(key='l')
    def _toggle_axis_scaley(self):
        """toggle log scale of y axis"""
        self.toggle_axis_scale('y')

    @register_shortcut(key='g')
    def _toggle_grid(self):
        """toggle axis grid"""
        self.curr_axes.grid()
        self.draw(new_data=False)


class PlotToolbar(NavigationToolbar2QT):
    toolitems = (
      # name, tooltip, icon, method_name, all None is a seperator
      ('Home', 'Reset original view', 'home', 'home'),
      ('Pan', 'Pan with left mouse, zoom with right', 'move', 'pan'),
      ('Zoom', 'Zoom to rectangle', 'zoom_to_rect', 'zoom'),
      (None, None, None, None),
      ('Subplots', 'Config subplots', 'subplots', 'configure_subplots'),
      ('Save', 'Save to file', 'filesave', 'save_figure'),
      (None, None, None, None),
      ('Help', 'Plot widget help', 'helpcall', 'help_info'),
    )

    def __init__(self, canvas, parent=None):
        self.canvas = canvas
        super(PlotToolbar, self).__init__(canvas, parent)
        self.setIconSize(QSize(18, 18))

    def _icon(self, name):
        """Reimplement of the mpl function
        """
        nm = name.split('.')[0]
        if hasattr(icons, nm):
            return getattr(icons, nm)
        else:
            return super(PlotToolbar, self)._icon(name)

    def help_info(self):
        info = []
        for (key, _), (_, doc) in sorted(_SHORTCUTS.items()):
            info.append(': '.join([key, doc]))
        show_information("\n".join(info), title="Keyboard/mouse shortcuts:")

    @register_shortcut(key='home')
    def home(self, *args):
        """reset view, start auto zoom"""
        super(PlotToolbar, self).home(*args)
        self.canvas.figure.gca().set_autoscale_on(True)
        self.canvas.draw(new_data=False)

    @register_shortcut(key='ctrl+h')
    def toggle_visible(self):
        """hide/show toolbar"""
        self.setVisible(not self.isVisible())

    @register_shortcut(key='p')
    def pan(self):
        """activate pan mode"""
        super(PlotToolbar, self).pan()

    @register_shortcut(key='z')
    def zoom(self):
        """activate zoom mode"""
        super(PlotToolbar, self).zoom()

    @register_shortcut(key='ctrl+y')
    def forward(self):
        """redo pan or zoom operation"""
        super(PlotToolbar, self).forward()

    @register_shortcut(key='ctrl+z')
    def back(self):
        """undo pan or zoom operation"""
        super(PlotToolbar, self).back()

    @register_shortcut(key='ctrl+s')
    def save_figure(self):
        """save plot to file"""
        super(PlotToolbar, self).save_figure()
