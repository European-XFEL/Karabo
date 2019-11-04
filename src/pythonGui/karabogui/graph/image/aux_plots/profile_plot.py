from numpy import append
from PyQt5.QtGui import QTransform
from PyQt5.QtWidgets import QAction, QMenu
from pyqtgraph import PlotItem

from karabogui.graph.common.const import (
    AXIS_ITEMS, AXIS_X, AXIS_Y, ROTATION_FACTOR)
from karabogui.graph.common.api import (
    create_axis_items, make_brush, make_pen, get_default_pen)
from karabogui.graph.common.enums import AxisType
from ..aux_plots.items import AuxPlotViewBox
from ..tools.profiler import IntensityProfiler

LABEL_STYLE = {'color': '#000000', 'font-size': '12px'}


class BaseStepPlot(PlotItem):

    def __init__(self, orientation="top"):
        # Create axis items for different plot orientations
        if orientation == "top":
            # Normal plot orientation, where x-axis is shown in the bottom
            # and y-axis on the left of the plot
            self._shown_axes = ["bottom", "left"]
        else:
            # Plot orientation is on the left of the layout, thus the x-axis
            # must be shown on the top and the y-axis on the left of the plot.
            self._shown_axes = ["right", "top"]
        axis_items = create_axis_items(AxisType.AuxPlot, self._shown_axes)

        super(BaseStepPlot, self).__init__(axisItems=axis_items,
                                           viewBox=AuxPlotViewBox())
        self.orientation = orientation

        # Initialize plot data items
        self._line = self.plot([], pen=(0, 0, 255))
        self._superimposed = self.plot([], pen=(255, 0, 0))

        self._initialize_widget()
        self._pen = get_default_pen()
        self._brush = make_brush('b', 140)
        self._second_pen = make_pen('r', width=1)

    # ---------------------------------------------------------------------
    # Public methods

    def set_data(self, x_data, y_data):
        # Assuming that data is discrete, we add the difference to the last
        # value and feed it to the plot. This last is needed to plot all values
        # when stepMode=True, which is then not used.
        if len(x_data) <= 1 or len(y_data) <= 1:
            return

        offset = x_data[1] - x_data[0]
        self._line.setData(append(x_data, x_data[-1] + offset), y_data,
                           stepMode=True,
                           pen=self._pen, fillLevel=0,
                           brush=self._brush)

        # Adjust the y range to the data range to not show the pedestal.
        y_range = ("yRange" if self.orientation in ["top", "bottom"]
                   else "xRange")
        self.setRange(**{y_range: (y_data.min(), y_data.max())})

    def set_superimposed_data(self, x_data, y_data):
        self._superimposed.setData(x_data, y_data, pen=self._second_pen)

    def clear_data(self):
        self._line.setData([], [], fillLevel=None, stepMode=False)

    def set_axis(self, axis):
        """ Use if unsliced image axis is needed for further calculations """

    # ---------------------------------------------------------------------
    # Private methods

    def _initialize_widget(self):
        if self.orientation == "left":
            # The view and data must be rotated.
            self.vb.invertY()
            self.vb.invertX()
            angle = 270
            transform = QTransform()
            transform.rotate(angle)
            transform.scale(*ROTATION_FACTOR[angle])

            self._line.setTransform(transform)
            self._superimposed.setTransform(transform)

            # Add fixed width to not overlap with main plot axes
            self.getAxis("right").setFixedWidth(35)

        # Show y-axis label
        self.setLabel(self._shown_axes[1], "Intensity", **LABEL_STYLE)

        # Polish axes rendering
        for axis in AXIS_ITEMS:
            axis_item = self.getAxis(axis)
            axis_item.setZValue(0)
            axis_item.setVisible(True)
            axis_x = (AXIS_X if self.orientation in ["top", "bottom"]
                      else AXIS_Y)
            if axis in axis_x:
                axis_item.enableAutoSIPrefix(False)

        self.hideButtons()


class ProfilePlot(BaseStepPlot):
    """Beam Profile plot is a StepPlot that has the calculations..

    Insert more details here.
    """

    def __init__(self, orientation="top", **kwargs):
        super(ProfilePlot, self).__init__(orientation)
        self._profiler = IntensityProfiler(**kwargs)
        self._fitted = False

        # Add context menu
        menu = QMenu()
        self.enable_fit_action = QAction("Gaussian fitting", menu)
        self.enable_fit_action.setCheckable(True)
        menu.addAction(self.enable_fit_action)
        self.vb.set_menu(menu)

    def analyze(self, region):
        # Set axis to process
        axis = 1 if (self.orientation in ["left", "right"]) else 0

        # Check if region wrt to the axis is valid
        if not region.valid(axis):
            self.clear_data()
            return

        profiles = self._profiler.profile(region, axis=axis)
        if profiles is None:
            return

        self.set_data(*profiles)

        if self._fitted:
            self.set_superimposed_data(*self._profiler.fit())
            return self._profiler.analyze()

    def enable_fit(self, enabled):
        self._fitted = enabled
        self.enable_fit_action.setChecked(enabled)

        # Clear superimposed data
        if not enabled:
            self.set_superimposed_data([], [])

    def set_axis(self, axis):
        self._profiler.set_axis(axis)
