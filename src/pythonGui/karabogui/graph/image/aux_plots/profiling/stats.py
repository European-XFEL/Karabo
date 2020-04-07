from traits.api import Constant, Dict

from ..base.stats import BaseStats, header_row, table_row


class ProfileStats(BaseStats):

    # This contains statistics for both x- and y-axis.
    x_stats = Dict
    y_stats = Dict

    # HTML table format for the stats
    html_table = Constant(
        header_row(tabs=('x', 'y'))
        + table_row(header="Amplitude", tabs=("{x_ampl}", "{y_ampl}"))
        + table_row(header="Max Pos.",
                    tabs=("{x_maxpos}", "{y_maxpos}"))
        + table_row(header="FWHM", tabs=("{x_fwhm}", "{y_fwhm}")))

    # -----------------------------------------------------------------------
    # Private methods

    @staticmethod
    def _to_string(value):
        """Converts the numerical value into an readable number"""
        return "{:.3g}".format(value) if value is not None else "-"

    # -----------------------------------------------------------------------
    # Trait properties

    def _get_html(self):
        html = ''
        x_stats, y_stats = self.x_stats, self.y_stats
        if x_stats and y_stats:
            table = self.html_table.format(
                x_ampl=self._to_string(x_stats.get("amplitude")),
                x_maxpos=self._to_string(x_stats.get("max_pos")),
                x_fwhm=self._to_string(x_stats.get("fwhm")),
                y_ampl=self._to_string(y_stats.get("amplitude")),
                y_maxpos=self._to_string(y_stats.get("max_pos")),
                y_fwhm=self._to_string(y_stats.get("fwhm")))
            html = self.html_format.format(contents=table)
        return html
