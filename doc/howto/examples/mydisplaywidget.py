from PyQt4.QtGui import QDoubleSpinBox

from karabo_gui.util import SignalBlocker
from karabo_gui.widget import DisplayWidget
from karabo.middlelayer import Simple


class MyDisplayWidget(DisplayWidget):
    category = Simple # this widget will show numbers
    alias = "My cool widget" # this is shown in the context menu

    def __init__(self, box, parent):
        super(MyDisplayWidget, self).__init__(box)
        
        self.widget = QDoubleSpinBox(parent)

    def typeChanged(self, box):
        # Do some initialization for a new value type
        pass

    def valueChanged(self, box, value, timestamp=None):
        # Typically something like:
        with SignalBlocker(self.widget):
            self.widget.setValue(value)
