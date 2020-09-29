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
        return "{:.2f}".format(value).rstrip('0').rstrip('.')


CELL_WIDTH = 50


def table_header(header="&nbsp;", tabs=tuple()):
    """Composes an html header row"""
    header_html = f"<th align='left' width='{CELL_WIDTH}'>{header}</th>"
    tabs_html = "".join([f"<th align='left' width='{CELL_WIDTH}'>{tab}</th>"
                         for tab in tabs])
    return header_html + tabs_html


def table_body(header="&nbsp;", tabs=tuple()):
    """Composes an html table row"""
    header_html = f"<th align='left' width='{CELL_WIDTH}'>{header}</th>"
    tabs_html = "".join([f"<td width='{CELL_WIDTH}'>{tab}</td>"
                         for tab in tabs])
    return header_html + tabs_html


def table_row(*rows):
    rows = f"<td width='{CELL_WIDTH / 3}'></td>".join(rows)
    return f"<tr>{rows}</tr>"
