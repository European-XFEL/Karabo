from pyqtgraph import InfiniteLine, SignalProxy
from qtpy.QtCore import QObject, Slot

from karabogui import icons
from karabogui.graph.common.api import CoordsLegend, create_button


class CrossTargetController(QObject):
    def __init__(self, plotItem):
        """Controller for showing a cross target on a plot"""
        super(CrossTargetController, self).__init__()
        self.plotItem = plotItem

        self.v_line = None
        self.h_line = None
        self.proxy = None
        self.legend = None
        self.action_button = create_button(
            checkable=True,
            icon=icons.target,
            tooltip="Get a CrossTarget for the plot",
            on_clicked=self.toggle)

    @Slot(object)
    def mouseMoved(self, event):
        """Catch the mouseMove event on the plotItem and show coordinates"""
        pos = event[0]
        # using signal proxy turns original arguments into a tuple
        if self.plotItem.sceneBoundingRect().contains(pos):
            mousePoint = self.plotItem.vb.mapSceneToView(pos)
            x, y = mousePoint.x(), mousePoint.y()
            self.legend.set_value(x, y)
            self.v_line.setPos(x)
            self.h_line.setPos(y)
            self.legend.setVisible(True)
        else:
            self.legend.setVisible(False)

    # -----------------------------------------------------------------------
    # Public methods

    @Slot(bool)
    def toggle(self, state):
        if state:
            self.activate()
        else:
            self.deactivate()

    def activate(self):
        self.v_line = InfiniteLine(angle=90, movable=False)
        self.h_line = InfiniteLine(angle=0, movable=False)
        self.plotItem.addItem(self.v_line, ignoreBounds=True)
        self.plotItem.addItem(self.h_line, ignoreBounds=True)
        self.legend = CoordsLegend()
        self.legend.setParentItem(self.plotItem.vb)
        self.legend.anchor(itemPos=(1, 0), parentPos=(1, 0), offset=(-5, 5))
        self.proxy = SignalProxy(self.plotItem.scene().sigMouseMoved,
                                 rateLimit=50, slot=self.mouseMoved)

    def deactivate(self):
        self.proxy.disconnect()
        self.proxy = None
        for item in [self.h_line, self.v_line]:
            self.plotItem.removeItem(item)
            item.deleteLater()
        self.v_line = None
        self.h_line = None
        self.legend.setParentItem(None)
        self.legend.deleteLater()
        self.legend = None
