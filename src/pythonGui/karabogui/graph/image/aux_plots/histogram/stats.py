from traits.api import Constant

from ..base.stats import BaseStats, table_row


class HistogramStats(BaseStats):

    # HTML table format for the stats
    html_table = Constant(
        table_row(header="Count", tabs=("{count}",))
        + table_row(header="Mean", tabs=("{mean}",))
        + table_row(header="Minimum", tabs=("{min}",))
        + table_row(header="Maximum", tabs=("{max}",))
        + table_row(header="Std. Dev.", tabs=("{std}",))
        + table_row(header="Mode", tabs=("{mode}",))
        + table_row(header="Bins", tabs=("{bins}",))
        + table_row(header="Bin Width", tabs=("{bin_width}",)))

    # -----------------------------------------------------------------------
    # Private methods

    @staticmethod
    def _to_string(value):
        """Converts the numerical value into an readable number"""
        if value is None:
            return "-"
        return "{:.2f}".format(value).rstrip('0').rstrip('.')

    # -----------------------------------------------------------------------
    # Trait properties

    def _get_html(self):
        html = ''
        if self.stats:
            formatted_stats = {key: self._to_string(value)
                               for key, value in self.stats.items()}
            table = self.html_table.format(**formatted_stats)
            html = self.html_format.format(contents=table)
        return html
