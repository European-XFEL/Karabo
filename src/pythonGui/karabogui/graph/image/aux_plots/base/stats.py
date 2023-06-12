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
from traits.api import Constant, Dict, HasStrictTraits, Property, String


class BaseStats(HasStrictTraits):

    # Statistics from the analyzer
    stats = Dict

    # HTML table body
    html_format = Constant("<table style='font-size: 9px'>"
                           "{contents}</table>")

    # HTML table contents
    html_table = Constant('')

    # Rendered HTML
    html = Property(String)

    def _get_html(self):
        """Subclass to implement specific html composition"""
        html = ''
        if self.stats:
            html = self.html_table.format(**self.stats)
        return html

    # -----------------------------------------------------------------------
    # Private methods

    @staticmethod
    def _to_string(value):
        """Converts the numerical value into an readable number"""
        if value is None:
            return "-"
        return f"{value:.2f}".rstrip('0').rstrip('.')
