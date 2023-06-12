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
