from PyQt4.QtCore import QPoint
from PyQt4.QtGui import QFont
from pyqtgraph import asUnicode, functions as fn, ViewBox

from karabogui.graph.common.api import AxisItem
from karabogui.graph.common.const import X_AXIS_HEIGHT, Y_AXIS_WIDTH


class AuxPlotViewBox(ViewBox):

    """ There is a need to subclass the viewbox to prevent the wheel scroll
    from destroying the plot range."""

    def __init__(self):
        super(AuxPlotViewBox, self).__init__()
        self.setMouseEnabled(x=False, y=False)

    def raiseContextMenu(self, event):
        """Reimplemented function of PyQtGraph"""

        if self.menu is None or not self.menuEnabled():
            return
        pos = event.screenPos()
        self.menu.popup(QPoint(pos.x(), pos.y()))

    def set_menu(self, menu):
        self.menu = menu


class AuxPlotAxisItem(AxisItem):
    """AxisItem for the aux plots."""

    def __init__(self, orientation, has_ticks=True):
        super(AuxPlotAxisItem, self).__init__(orientation, has_ticks)
        self.setStyle(autoExpandTextSpace=False)
        font = QFont()
        font.setPixelSize(8)
        self.tickFont = font

    # ---------------------------------------------------------------------
    # pyqtgraph methods

    def _updateWidth(self):
        """Patching to match known width of main plot"""
        if self.label.isVisible():
            self.setFixedWidth(Y_AXIS_WIDTH)
            self.picture = None
        else:
            super(AuxPlotAxisItem, self)._updateWidth()

    def _updateHeight(self):
        """Patching to match known height of main plot"""
        if self.label.isVisible():
            self.setFixedHeight(X_AXIS_HEIGHT)
            self.picture = None
        else:
            super(AuxPlotAxisItem, self)._updateHeight()

    def updateAutoSIPrefix(self):
        """Patching to scale tick values after 10^3"""
        if self.label.isVisible():
            (scale, prefix) = fn.siScale(max(abs(self.range[0] * self.scale),
                                             abs(self.range[1] * self.scale)))
            if self.labelUnits == '' and prefix == 'k':
                scale = 1.0
                prefix = ''
            self.setLabel(unitPrefix=prefix)
        else:
            scale = 1.0

        self.autoSIPrefixScale = scale
        self.picture = None
        self.update()

    def labelString(self):
        """Patching to improve label display of scaled axes (e.g. x 10^6)"""
        if self.labelUnits != '':
            units = asUnicode('(%s%s)') % (asUnicode(self.labelUnitPrefix),
                                           asUnicode(self.labelUnits))
        else:
            units = ''

        s = asUnicode('%s %s') % (asUnicode(self.labelText), asUnicode(units))
        style = ';'.join(['%s: %s' % (k, self.labelStyle[k])
                          for k in self.labelStyle])
        main = asUnicode("<span align='center' style='%s'>%s</span>") \
            % (style, asUnicode(s))

        if not self.autoSIPrefix or self.autoSIPrefixScale == 1.0:
            scaling = ''
        else:
            scaling = asUnicode('<center>(\u00D7 %g)</center>') \
                      % (1.0 / self.autoSIPrefixScale)

        scale_style = []
        for k in self.labelStyle:
            if k == "font-size":
                font_size = int(self.labelStyle[k][:-2].strip()) - 2
                scale_style.append("%s: %s" % (k, str(font_size) + "px"))
            else:
                scale_style.append("%s: %s" % (k, self.labelStyle[k]))
        style = ';'.join(scale_style)
        scale = asUnicode("<span align='center' style='%s'>%s</span>") \
            % (style, asUnicode(scaling))

        return asUnicode('%s %s') % (asUnicode(main), asUnicode(scale))
