from PyQt4.QtCore import QPoint
from pyqtgraph import ViewBox


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
