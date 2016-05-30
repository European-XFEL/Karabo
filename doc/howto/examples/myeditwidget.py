from PyQt4.QtGui import QDoubleSpinBox

from karabo_gui.util import SignalBlocker
from karabo_gui.widget import EditableWidget
from karabo.middlelayer import Simple


class MyEditWidget(EditableWidget):
    category = Simple # this widget will show numbers
    alias = "My cool widget" # this is shown in the context menu

    def __init__(self, box, parent):
        super(MyEditWidget, self).__init__(box)
        
        self.widget = QDoubleSpinBox(parent)

    @property
    def value(self):
        """ The current value of the widget is returned.
        """
        return self.widget.value()

    def typeChanged(self, box):
        # Do some initialization for a new value type
        pass

    def valueChanged(self, box, value, timestamp=None):
        # Typically something like:
        with SignalBlocker(self.widget):
            self.widget.setValue(value)
