from numpy import append

from karabogui.graph.common.api import make_brush, make_pen, get_default_pen

from ..plot import BaseAuxPlotItem


class StepPlot(BaseAuxPlotItem):

    def __init__(self, orientation="top"):
        super(StepPlot, self).__init__(orientation, y_label="Intensity")

        # Initialize plot data items
        line = self.plot([], [])
        superimposed = self.plot([], [])
        self._data_items = [line, superimposed]
        self._adjust_to_orientation()

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
        profile_plot = self._data_items[0]
        profile_plot.setData(append(x_data, x_data[-1] + offset), y_data,
                             stepMode=True, pen=self._pen, fillLevel=0,
                             brush=self._brush)

        # Adjust the y range to the data range to not show the pedestal.
        y_range = ("yRange" if self.orientation in ["top", "bottom"]
                   else "xRange")
        self.setRange(**{y_range: (y_data.min(), y_data.max())})

    def set_superimposed_data(self, x_data, y_data):
        self._data_items[1].setData(x_data, y_data, pen=self._second_pen)
