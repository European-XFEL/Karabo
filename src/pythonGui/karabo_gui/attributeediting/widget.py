from PyQt4.QtCore import pyqtSignal

from karabo_gui.widget import Widget


class AttributeWidget(Widget):
    """All widgets with which one can edit attributes should inherit from
    this class.
    """
    factories = {}
    signalEditingFinished = pyqtSignal(object, object)

    def __init__(self, box, parent=None):
        super(AttributeWidget, self).__init__(box)
        self.attr_name = None
        box.configuration.boxvalue.state.signalUpdateComponent.connect(
            self.updateStateSlot)

    def onEditingFinished(self, value):
        self.signalEditingFinished.emit(self.boxes[0], value)

    def attributeValueChanged(self, value):
        """ Called when the attribute gets assigned a value.

        Generally, this is only handling initialization.
        """
        raise NotImplementedError

    def valueChanged(self, box, value, timestamp=None):
        # `value` is the box's value, replace it with the attribute value.
        assert self.attr_name is not None
        value = getattr(box.descriptor, self.attr_name)
        self.attributeValueChanged(value)
