from abc import abstractmethod

from PyQt5.QtCore import QPoint
from PyQt5.QtGui import QTransform
from PyQt5.QtWidgets import QMenu
from pyqtgraph import PlotItem, ViewBox
from traits.api import (
    ABCHasStrictTraits, Constant, Instance, Property, String)

from karabogui.graph.common.const import (
    AXIS_ITEMS, AXIS_X, AXIS_Y, ROTATION_FACTOR)
from karabogui.graph.common.api import create_axis_items
from karabogui.graph.common.enums import AxisType


LABEL_STYLE = {'color': '#000000', 'font-size': '12px'}

SHOWN_AXES = {
    # Normal plot orientation, where x-axis is shown in the bottom
    # and y-axis on the left of the plot.
    "top": ["bottom", "left"],

    # Plot orientation is on the left of the layout, thus the x-axis
    # must be shown on the top and the y-axis on the left of the plot.
    "left": ["right", "top"]
}


class AuxPlotViewBox(ViewBox):

    """ There is a need to subclass the viewbox to prevent the wheel scroll
    from destroying the plot range."""

    def __init__(self):
        super(AuxPlotViewBox, self).__init__()
        self.setMouseEnabled(x=False, y=False)

    def raiseContextMenu(self, event):
        """Reimplemented function of PyQtGraph"""

        if self.menu is None or not self.menuEnabled():
            return
        pos = event.screenPos()
        self.menu.popup(QPoint(pos.x(), pos.y()))

    def set_menu(self, menu):
        self.menu = menu


class AuxPlotItem(PlotItem):

    def __init__(self, orientation="top", axisItems=None):
        super(AuxPlotItem, self).__init__(axisItems=axisItems,
                                          viewBox=AuxPlotViewBox())

        # Initialize plot item
        if orientation == "left":
            # Add fixed width to not overlap with main plot axes
            self.getAxis("right").setFixedWidth(35)
            # The view and must be rotated.
            self.vb.invertY()
            self.vb.invertX()
        elif orientation == "top":
            # Add fixed width to not overlap with main plot axes
            self.getAxis("bottom").setFixedHeight(20)

        # Polish axes rendering
        for axis in AXIS_ITEMS:
            axis_item = self.getAxis(axis)
            axis_item.setZValue(0)
            axis_item.setVisible(True)
            axis_x = (AXIS_X if orientation in ["top", "bottom"] else AXIS_Y)
            if axis in axis_x:
                axis_item.enableAutoSIPrefix(False)

        self.hideButtons()

    def setLabel(self, axis, text=None, **kwargs):
        super(AuxPlotItem, self).setLabel(axis, text=text, **LABEL_STYLE)


class BasePlot(ABCHasStrictTraits):

    plotItem = Instance(AuxPlotItem, clean_up=True)
    orientation = String

    x_label = Constant('')
    y_label = Constant('')

    menu = Property(Instance(QMenu))

    def __init__(self, **traits):
        super(BasePlot, self).__init__(**traits)

        # Create axis items for different plot orientations
        shown_axes = SHOWN_AXES[self.orientation]
        axis_items = create_axis_items(AxisType.AuxPlot, shown_axes)

        # Instantiate plotItem
        plotItem = AuxPlotItem(orientation=self.orientation,
                               axisItems=axis_items)
        self.plotItem = plotItem

        # Set labels only if there's existing value
        if self.x_label:
            plotItem.setLabel(shown_axes[0], self.x_label)
        if self.y_label:
            plotItem.setLabel(shown_axes[1], self.y_label)

    def _add_plot_item(self):
        item = self.plotItem.plot([], [])

        # Transform added plot item if the orientation is unconventional
        # (left, right, bottom). For now, left is only implemented
        if self.orientation == "left":
            angle = 270
            transform = QTransform()
            transform.rotate(angle)
            transform.scale(*ROTATION_FACTOR[angle])
            item.setTransform(transform)

        return item

    def _set_menu(self, menu):
        self.plotItem.vb.set_menu(menu)

    def destroy(self):
        """Destroy the plotItem cleanly"""
        self.plotItem.deleteLater()

    @abstractmethod
    def set_data(self, x_data, y_data):
        """Used by subclass to tune the data and the graphical parameters
           according to its specifications
           (e.g., histograms have gradient fill)"""

    def clear_data(self):
        for data_item in self.plotItem.dataItems:
            data_item.setData([], [], fillLevel=None, stepMode=False)