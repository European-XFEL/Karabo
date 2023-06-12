# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
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
