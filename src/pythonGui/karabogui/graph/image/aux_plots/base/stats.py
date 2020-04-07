from traits.api import Constant, Dict, HasStrictTraits, Property, String


class BaseStats(HasStrictTraits):

    # Statistics from the analyzer
    stats = Dict

    # HTML table body
    html_format = Constant("<table style='font-size: 8px'><tbody>"
                           "{contents}"
                           "</tbody></table>")

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

    @staticmethod
    def _to_string(value):
        """Subclass to implement specific number formatting"""
        return str(value)


def header_row(header="&nbsp;", tabs=tuple()):
    """Composes an html header row"""
    header_html = "<th align='left'>{}</th>".format(header)
    tabs_html = "".join(["<th>{}</th>".format(tab) for tab in tabs])
    return "<tr>" + header_html + tabs_html + "</tr>"


def table_row(header="&nbsp;", tabs=tuple()):
    """Composes an html table row"""
    header_html = "<th align='left'>{}</th>".format(header)
    tabs_html = "".join(["<td>{}</td>".format(tab) for tab in tabs])
    return "<tr>" + header_html + tabs_html + "</tr>"
