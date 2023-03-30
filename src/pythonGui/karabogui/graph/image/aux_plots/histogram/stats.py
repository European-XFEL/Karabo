from traits.api import Constant

from karabogui.graph.common.formatting import table_body, table_row

from ..base.stats import BaseStats


class HistogramStats(BaseStats):

    # HTML table format for the stats
    html_table = Constant(
        "<b><u>Histogram Statistics</b></u><br/>"
        + table_row(table_body(header="Count", tabs=("{count}",)))
        + table_row(table_body(header="Mean", tabs=("{mean}",)))
        + table_row(table_body(header="Minimum", tabs=("{min}",)))
        + table_row(table_body(header="Maximum", tabs=("{max}",)))
        + table_row(table_body(header="Std. Dev.", tabs=("{std}",)))
        + table_row(table_body(header="Mode", tabs=("{mode}",)))
        + table_row(table_body(header="Bins", tabs=("{bins}",)))
        + table_row(table_body(header="Bin Width", tabs=("{bin_width}",)))
    )

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
