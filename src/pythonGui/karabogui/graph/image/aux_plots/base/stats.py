# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
