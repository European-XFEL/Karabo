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
