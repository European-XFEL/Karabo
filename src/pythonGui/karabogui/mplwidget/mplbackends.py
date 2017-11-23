from matplotlib.backends.backend_qt4agg import (
    FigureCanvasQTAgg, NavigationToolbar2QT
)
from matplotlib.backend_tools import cursors
from matplotlib.figure import Figure
from matplotlib.lines import Line2D
from PyQt4.QtCore import Qt, QSize
from PyQt4.QtGui import QCursor, QMenu, QSizePolicy

from karabogui import icons
from karabogui.messagebox import show_information
from . import const
from .tools import DataCursor
from .utils import register_shortcut, _SHORTCUTS


class FigureCanvas(FigureCanvasQTAgg):

    def __init__(self, *, parent=None, **kwargs):
        # fig is a container which can hold any (number) of axes
        fig = Figure(figsize=const.PLOTSIZE, facecolor=None, dpi=const.DPI)
        fig.patch.set_alpha(0)  # transparent background
        super(FigureCanvas, self).__init__(fig)
        self.curr_axes = fig.add_subplot(111)

        # set minimum size on the canvas, the outer widget can be shrinked,
        # but the plot won't become unreadable. this is useful for saving to
        # file.
        self.setMinimumHeight(const.MINPLOTHIGHT)
        self.setMinimumWidth(const.MINPLOTWIDTH)
        self.setSizePolicy(QSizePolicy.Expanding,
                           QSizePolicy.Expanding)
        self.setFocusPolicy(Qt.ClickFocus)
        self.toolbar = None  # this will be set by the toolbar class
        self.popmenu = QMenu(self)
        self.highlighted = None

        self.mpl_connect('button_press_event', self.on_mouse_press)
        self.mpl_connect('button_release_event', self.on_mouse_release)
        self.mpl_connect('pick_event', self.on_pick)

    def draw(self, rescale=True):
        if rescale:
            self.curr_axes.relim(visible_only=True)  # recalculate data limit
            self.curr_axes.autoscale_view()
        super(FigureCanvas, self).draw()

    def on_mouse_press(self, event):
        if not event.inaxes and event.dblclick:
            self.toolbar.edit_parameters()
            return

    def on_mouse_release(self, event):
        tb = self.toolbar
        if event.button == const.MOUSE_RIGHT_BUTTON:
            if tb.mode or tb.data_cursor_visible:
                return
            elif event.inaxes:
                self.popmenu.exec(QCursor.pos())

    def on_pick(self, event):
        if event.mouseevent.button == const.MOUSE_LEFT_BUTTON:
            picked = event.artist
            if isinstance(picked, Line2D):
                if self.highlighted is picked:
                    picked.set_markeredgewidth(0)
                    self.highlighted = None
                else:
                    if self.highlighted is not None:
                        self.highlighted.set_markeredgewidth(0)
                    picked.set_markeredgewidth(2)
                    self.highlighted = picked
            self.draw(rescale=False)

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
            self.draw(rescale=False)

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
        self.draw(rescale=False)

    def init_popmenu(self):
        """Copy toolbar buttons to right click menu, skip Pan and Zoom buttons
        because they will hijack mouse click reactions once activated.
        """
        for action in self.toolbar.actions():
            name = action.text()
            if name in ('Pan', 'Zoom'):
                continue
            elif not name:
                self.popmenu.addSeparator()
                continue
            self.popmenu.addAction(action)

    def adjust_layout(self):
        rect = const.FIGURE_FULL
        if len(self.curr_axes.artists):
            rect = const.FIGURE_WITH_ARTISTS
        self.figure.tight_layout(rect=rect)
        self.draw(rescale=False)


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
        super(PlotToolbar, self).__init__(canvas, parent, coordinates=False)
        self.figure = canvas.figure
        self.setIconSize(QSize(18, 18))
        self.data_cursor = None
        self.data_cursor_visible = False
        self._cid = {}  # keep track of mpl event connected functions

    def _icon(self, name):
        """Reimplement of the mpl function
        """
        nm = name.split('.')[0]
        if hasattr(icons, nm):
            return getattr(icons, nm)
        else:
            return super(PlotToolbar, self)._icon(name)

    @register_shortcut(key='ctrl+h')
    def help_info(self):
        info = []
        for (key, _), (_, doc) in sorted(_SHORTCUTS.items()):
            info.append(': '.join([key, doc]))
        show_information("\n".join(info), title="Keyboard/mouse shortcuts:")

    @register_shortcut(key='home')
    def home(self, *args):
        """reset view, start auto zoom"""
        self.figure.gca().set_autoscale_on(True)
        self.canvas.draw(rescale=True)

    @register_shortcut(key='p')
    def pan(self):
        """activate pan mode"""
        if self.data_cursor_visible:
            self.crosshair()
        super(PlotToolbar, self).pan()
        # self.mode is defined in base class
        if self.mode == 'pan/zoom':
            # MPL don't change cursor until mouse move, we do it to give user
            # a quick visual response.
            super(PlotToolbar, self).set_cursor(cursors.MOVE)
        else:
            super(PlotToolbar, self).set_cursor(cursors.POINTER)

    @register_shortcut(key='z')
    def zoom(self):
        """activate zoom mode"""
        if self.data_cursor_visible:
            self.crosshair()
        super(PlotToolbar, self).zoom()
        if self.mode == 'zoom rect':
            super(PlotToolbar, self).set_cursor(cursors.SELECT_REGION)
        else:
            super(PlotToolbar, self).set_cursor(cursors.POINTER)

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

    @register_shortcut(key='c')
    def crosshair(self, event=None):
        """toggle cross hair mode"""
        # this mode is always triggered by key press, we relay the key press
        # event because it (wierdly) contains mouse pos. This is needed for the
        # first crosshair draw.
        data_cursor = self.data_cursor
        if self.data_cursor_visible:
            self.data_cursor_visible = data_cursor.set_visible(False, event)
            self.canvas.mpl_disconnect(self._cid.pop('crosshair'))
            return

        cmode = self.mode
        if cmode == 'pan/zoom':
            self.pan()
        elif cmode == 'zoom rect':
            self.zoom()

        canvas = self.canvas
        cax = canvas.curr_axes
        if data_cursor is None:
            data_cursor = self.data_cursor = DataCursor(cax, visible=False,
                                                        color='r')
        elif data_cursor.ax != cax:
            self.figure.texts.remove(data_cursor.text)
            data_cursor = self.data_cursor = DataCursor(cax, visible=False,
                                                        color='r')
        self.data_cursor_visible = data_cursor.set_visible(True, event)
        self._cid['crosshair'] = canvas.mpl_connect('motion_notify_event',
                                                    data_cursor.showdata)
