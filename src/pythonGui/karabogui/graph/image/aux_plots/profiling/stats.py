# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from traits.api import Constant, Dict

from karabogui.graph.common.formatting import (
    table_body, table_header, table_row)

from ..base.stats import BaseStats


class ProfileStats(BaseStats):

    # This contains statistics for both x- and y-axis.
    x_stats = Dict
    y_stats = Dict

    # HTML table format for the stats
    html_table = Constant(
        "<b><u>Beam Profile Statistics</b></u><br/>"
        + table_row(table_header(tabs=('x', 'y')))
        + table_row(table_body(header="Amplitude",
                               tabs=("{x_ampl}", "{y_ampl}")))
        + table_row(table_body(header="Max Pos.",
                               tabs=("{x_maxpos}", "{y_maxpos}")))
        + table_row(table_body(header="FWHM",
                               tabs=("{x_fwhm}", "{y_fwhm}")))
    )

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
