from PyQt5.QtWidgets import QMenu
from pyqtgraph import PlotItem
from traits.api import (
    Bool, Event, Dict, HasStrictTraits, Instance, Property, String, Type)

from .analyzer import BaseAnalyzer
from .plot import BasePlot


class ControllerAggregator(HasStrictTraits):
    """Aggregates multiple plot controllers to a unified interface for the base
       image plot to communicate with."""

    # Override to specify the plot class and its orientation
    # `{orientation: plot_klass}`
    contents = Dict

    # The dictionary which contains the instantiated aux plot controllers
    # `{orientation: plot_klass()}`
    controllers = Dict

    # List of plotItems from the aux plots
    plotItems = Property

    # Show the stats on the labelItem
    show_stats = Bool(False)

    # Forwards the stats to the aux plot controller
    stats = Event

    # Shared menu for the aux plot contents
    menu = Property(Instance(QMenu))

    def __init__(self, config=None, **traits):
        super(ControllerAggregator, self).__init__(**traits)

        for orientation, plot_klass in self.contents.items():
            if config is None:
                config = {}
            controller = plot_klass(orientation=orientation, **config)
            self.controllers[orientation] = controller

    # -----------------------------------------------------------------------
    # Public methods

    def process(self, region):
        # The default process implementation. No fancy config needed
        for plot in self.controllers.values():
            plot.process(region)

    def link_view(self, view):
        """Connects the image plot viewbox to the respective aux plots"""

    def set_axes(self, x_data, y_data):
        """Set the transformed image axes data to the respective aux plots"""

    def destroy(self):
        """Destroy the children members and disconnect trait handlers"""
        for controller in self.controllers.values():
            controller.destroy()

    # -----------------------------------------------------------------------
    # Trait properties

    def _get_plotItems(self):
        return [plot.plotItem for plot in self.controllers.values()]

    def _get_menu(self):
        """See if there's shared menu"""


class BaseController(HasStrictTraits):
    """Aux plot controller."""

    # The analyzer class performs the calculations/data processing.
    # This is usually overwritten and initialized on the subclass
    analyzer = Instance(BaseAnalyzer)

    # The plot class is overwritten on the subclass to specify which plot
    # item controller to use.
    plot_klass = Type(BasePlot)

    # The plot item controller class. This is instantiated with the specified
    # `plot_klass` from the subclass.
    plot = Instance(BasePlot)

    # The pyqtgraph plotItem from the `plotItem`.
    plotItem = Property(Instance(PlotItem))

    # The orientation of the plot. ["top", "left", "right", "bottom"]
    orientation = String

    def __init__(self, **traits):
        super(BaseController, self).__init__(**traits)
        self.plot = self.plot_klass(orientation=self.orientation)

    # -----------------------------------------------------------------------
    # Public methods

    def process(self, region, **config):
        """Process is made up of two parts:
           1. Analyze the input data
           2. Plot the resulting data"""

        if not region.valid():
            self.clear_data()
            return

        x_data, y_data = self.analyzer.analyze(region)
        self.plot.set_data(x_data, y_data)

    def clear_data(self):
        self.plot.clear_data()
        self.analyzer.clear_data()

    def link_view(self, axis=0, view=None):
        """Link the image plot viewbox and the aux plot wrt specified axis"""
        self.plotItem.vb.linkView(axis, view)

    def destroy(self):
        """Destroy the children cleanly"""
        self.plot.destroy()

    # -----------------------------------------------------------------------
    # Trait properties

    def _get_plotItem(self):
        return self.plot.plotItem