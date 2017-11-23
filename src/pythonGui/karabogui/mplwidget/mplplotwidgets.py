from matplotlib._pylab_helpers import Gcf
from matplotlib.legend import Legend
from PyQt4.QtGui import QStackedLayout, QWidget

from .const import MOUSE_LEFT_BUTTON, PICK_RADIUS, UPPER_RIGHT
from .mplbackends import FigureCanvas, PlotToolbar
from .utils import _SHORTCUTS, auto_line2D


class MplWidgetBase(QWidget):
    """This is the base class to generate plots using MPL backend.
    The canvas handles system interactions (draw, react to key/mouse press).
    Plot tools provide useful interactive functionalities.
    """

    def __init__(self, parent=None):
        super(MplWidgetBase, self).__init__(parent=parent)
        self._canvas = FigureCanvas(parent=self)
        self._toolbar = PlotToolbar(self._canvas, parent=self)
        self._canvas.init_popmenu()

        # use mpl's key event to dispatch key shortcuts
        self._key_event_receiver = {inst.__class__.__name__: inst for inst in
                                    (self, self._canvas, self._toolbar)}
        self._canvas.mpl_connect('key_press_event', self.on_key_press)

        # XXX: The plot flickers if the canvas doesn't occupy the whole
        # widget.
        layout = QStackedLayout(self)
        layout.addWidget(self._canvas)
        layout.addWidget(self._toolbar)

    def on_key_press(self, event):
        for klass in self._key_event_receiver:
            funcname, _ = _SHORTCUTS.get((event.key, klass), (None, None))
            if funcname is not None:
                func = getattr(self._key_event_receiver[klass], funcname)
                if 'event' in func.__code__.co_varnames:
                    # relay the event if func has it in its arg
                    func(event)
                else:
                    func()
                break

    def destroy(self):
        """Clean up all figures"""
        Gcf.destroy_all()


class MplCurvePlot(MplWidgetBase):

    def __init__(self, parent=None, legend=False):
        super(MplCurvePlot, self).__init__(parent=parent)
        # An iterator generates a new Line2D instance with a different style
        self.line_generator = auto_line2D()
        self.curves = {}
        self.use_legend = legend
        if legend:
            self.legend_LUT = {}
            self._canvas.mpl_connect('pick_event', self._on_pick_legend)

    def new_curve(self, **kwargs):
        """Register a new curve, return its label for later reference"""
        label = kwargs.get('label', r'line{}')
        if r'{}' in label:
            label = label.format(len(self.curves))
        exist = label in self.curves
        if exist:
            return label
        line = next(self.line_generator)
        line.set_label(label)
        self.curves[label] = line
        axes = self._canvas.curr_axes
        axes.add_line(line)
        if self.use_legend:
            self._gen_legend()
        return label

    def update_curve(self, xdata, ydata, label):
        """Set data to a labeled curve, if label doesn't exist, register
        a new curve.
        """
        if len(xdata) != len(ydata):
            return
        line = self.curves.get(label)
        if line is None:
            label = self.new_curve(label=label)
            line = self.curves[label]
        line.set_data(xdata, ydata)
        self._canvas.draw()

    def axes_call(self, func, *arg, **kwargs):
        axesfunc = getattr(self._canvas.figure.gca(), func)
        axesfunc(*arg, **kwargs)
        if func in ('set_xlabel', 'set_ylabel'):
            self._canvas.adjust_layout()

    # --------------------------------------------------------
    # private functions

    def _gen_legend(self):
        """Generate interactive legend, prepare a LUT for picking"""
        axes = self._canvas.curr_axes
        transform = axes.figure.transFigure
        bbox = UPPER_RIGHT
        if self.legend_LUT:
            for obj in axes.artists:
                if isinstance(obj, Legend):
                    obj.remove()
                    break

        lines = list(self.curves.values())
        labels = [ln.get_label() for ln in lines]
        leg = Legend(axes, lines, labels, bbox_to_anchor=bbox,
                     bbox_transform=transform)
        leg.draggable(True, update='loc')

        axes.add_artist(leg)
        self._canvas.adjust_layout()

        self.legend_LUT.clear()
        for legline, origline in zip(leg.get_lines(), lines):
            legline.set_picker(PICK_RADIUS)
            self.legend_LUT[legline] = origline

    def _on_pick_legend(self, event):
        if event.mouseevent.button != MOUSE_LEFT_BUTTON:
            return
        picked = event.artist
        ln = self.legend_LUT.get(picked)
        if ln is None:
            return
        vis = not ln.get_visible()
        ln.set_visible(vis)
        if vis:
            picked.set_alpha(1.0)
        else:
            picked.set_alpha(0.2)
        self._canvas.draw()
