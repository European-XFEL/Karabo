from .histogram import Histogram
from .plot import HistogramPlot
from ..base import BaseAuxPlotController

HTML_TABLE = """
<table style='font-size:8px'>
<tbody>
<tr>
<th align="left">Count</th><td>{count}</td>
</tr>
<tr>
<th align="left">Mean</th><td>{mean}</td>
</tr>
<tr>
<th align="left">Minimum</th><td>{min}</td>
</tr>
<tr>
<th align="left">Maximum</th><td>{max}</td>
</tr>
<tr>
<th align="left">Std. Dev.</th><td>{std}</td>
</tr>
<tr>
<th align="left">Mode</th><td>{mode}</td>
</tr>
<tr>
<th align="left">Bins</th><td>{bins}</td>
</tr>
<tr>
<th align="left">Bin Width</th><td>{bin_width}</td>
</tr>
</tbody>
</table>
"""


def _to_string(value):
    if value is None:
        return "-"
    return "{:.2f}".format(value).rstrip('0').rstrip('.')


class HistogramController(BaseAuxPlotController):
    """Histograms counts the image z-value (intensity) """

    def __init__(self, **config):
        super(HistogramController, self).__init__(**config)
        plot = HistogramPlot()
        plot.show_stats_action.triggered.connect(self._show_stats)

        self._plots = [(plot, Histogram())]
        self._stats_enabled = True

    def analyze(self, region):
        plot, analyzer = self._plots[0]

        if not region.valid():
            plot.clear_data()
            analyzer.clear_data()
            return

        edges, hist = analyzer.analyze(region.flatten())
        plot.set_data(edges, hist)

    def get_html(self):
        if not self._stats_enabled:
            return ''

        stats = self.analyzers[0].get_stats()
        if stats is None:
            return ''
        return HTML_TABLE.format(**{key: _to_string(value)
                                    for key, value in stats.items()})

    def set_colormap(self, colormap):
        self.plots[0].set_colormap(colormap)

    def set_levels(self, levels):
        self.plots[0].set_levels(levels)

    # ---------------------------------------------------------------------
    # Private methods

    def _show_stats(self, enabled):
        self._stats_enabled = enabled
        stats = self.get_html() if enabled else ''
        self.showStatsRequested.emit(stats)
