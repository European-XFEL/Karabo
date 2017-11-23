#############################################################################
# Author: <chen.xu@xfel.eu>
# Created on November 1, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import matplotlib

MOUSE_LEFT_BUTTON = 1
MOUSE_RIGHT_BUTTON = 3

# This is the MPL v2.0 default color cycle sans red and black
# https://matplotlib.org/_images/dflt_style_changes-1.png
COLORS = ('#1f77b4', '#ff7f0e', '#2ca02c', '#9467bd',
          '#8c564b', '#e377c2', '#bcbd22', '#17becf')
DPI = 100
FONTSIZE = 8
LINE_STYLES = ('-', '--', ':', '-.')
LINE_WIDTH = 1.5  # dots
MARKER_EDGE_WIDTH = 0
MINPLOTHIGHT = 200  # pixels
MINPLOTWIDTH = 300
PICK_RADIUS = 1.5  # dots
PLOTSIZE = (5, 3)  # default plot size in inches

# all values below are portion of the figure
UPPER_RIGHT = (1.0, 1.0)
FIGURE_FULL = (0, 0, 1, 1)
FIGURE_WITH_ARTISTS = (0, 0, 0.8, 1.0)


matplotlib.rcParams.update({'font.size': FONTSIZE,
                            'grid.color': '#b0b0b0',
                            'grid.linestyle': ':',
                            'grid.linewidth': 1.0,
                            'grid.alpha': 0.8})
