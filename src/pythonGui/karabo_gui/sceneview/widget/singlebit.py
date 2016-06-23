from karabo_gui.editablewidgets.singlebit import SingleBit
from .base import BaseWidgetContainer


class _SingleBitWrapper(SingleBit):
    """ A wrapper around SingleBit
    """
    def __init__(self, model, box, parent):
        super(_SingleBitWrapper, self).__init__(box, parent)
        self.model = model

        # Initialize the widget
        super(_SingleBitWrapper, self)._setBit(self.model.bit)

    def _setBit(self, bit):
        super(_SingleBitWrapper, self)._setBit(bit)
        self.model.bit = bit


class SingleBitContainer(BaseWidgetContainer):
    def _create_widget(self, boxes):
        return _SingleBitWrapper(self.model, boxes[0], self)
