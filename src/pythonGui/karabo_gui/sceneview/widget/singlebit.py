from karabo_gui.displaywidgets.singlebit import SingleBit
from .base import BaseWidgetContainer


class _SingleBitWrapper(SingleBit):
    """ A wrapper around SingleBit
    """
    def __init__(self, model, box, parent):
        super(_SingleBitWrapper, self).__init__(box, parent)
        self.model = model

        # Initialize the widget
        super(_SingleBitWrapper, self)._setBit(self.model.bit)
        super(_SingleBitWrapper, self)._setInvert(self.model.invert)

    def _setBit(self, bit):
        super(_SingleBitWrapper, self)._setBit(bit)
        self.model.bit = bit

    def _setInvert(self, flag):
        super(_SingleBitWrapper, self)._setInvert(flag)
        self.model.invert = flag


class SingleBitContainer(BaseWidgetContainer):
    def _create_widget(self, boxes):
        return _SingleBitWrapper(self.model, boxes[0], self)
