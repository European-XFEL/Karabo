from pyqtgraph import PlotItem
from PyQt5.QtGui import QTransform

from karabogui.graph.common.const import (
    AXIS_ITEMS, AXIS_X, AXIS_Y, ROTATION_FACTOR)
from karabogui.graph.common.api import create_axis_items
from karabogui.graph.common.enums import AxisType

from .viewbox import AuxPlotViewBox


LABEL_STYLE = {'color': '#000000', 'font-size': '12px'}


class BaseAuxPlotItem(PlotItem):

    def __init__(self, orientation="top", **config):
        super(BaseAuxPlotItem, self).__init__()

        # Create axis items for different plot orientations
        if orientation == "top":
            # Normal plot orientation, where x-axis is shown in the bottom
            # and y-axis on the left of the plot
            shown_axes = ["bottom", "left"]
        else:
            # Plot orientation is on the left of the layout, thus the x-axis
            # must be shown on the top and the y-axis on the left of the plot.
            shown_axes = ["right", "top"]
        axis_items = create_axis_items(AxisType.AuxPlot, shown_axes)

        super(BaseAuxPlotItem, self).__init__(axisItems=axis_items,
                                              viewBox=AuxPlotViewBox())
        self.orientation = orientation

        # Initialize plot item
        if orientation == "left":
            # Add fixed width to not overlap with main plot axes
            self.getAxis("right").setFixedWidth(35)
        elif orientation == "top":
            # Add fixed width to not overlap with main plot axes
            self.getAxis("bottom").setFixedHeight(20)

        # Set labels
        x_label = config.get("x_label")
        if x_label is not None:
            self.setLabel(shown_axes[0], x_label, **LABEL_STYLE)
        y_label = config.get("y_label")
        if y_label is not None:
            self.setLabel(shown_axes[1], y_label, **LABEL_STYLE)

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

    def _adjust_to_orientation(self):
        if self.orientation == "left":
            # The view and data must be rotated.
            self.vb.invertY()
            self.vb.invertX()
            angle = 270
            transform = QTransform()
            transform.rotate(angle)
            transform.scale(*ROTATION_FACTOR[angle])

            for data_item in self._data_items:
                data_item.setTransform(transform)

    def set_data(self, x_data, y_data):
        """Used by subclass to tune the data and the graphical parameters
           according to its specifications
           (e.g., histograms have gradient fill)"""
        raise NotImplementedError

    def clear_data(self):
        for data_item in self._data_items:
            data_item.setData([], [], fillLevel=None, stepMode=False)
