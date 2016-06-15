from PyQt4.QtGui import QLayout, QLayoutItem

from .utils import save_painter_state


class _ShapeLayoutItemChild(QLayoutItem):
    """ A private QLayoutItem for the ShapeLayoutItem
    """
    def __init__(self, shape):
        super(_ShapeLayoutItemChild, self).__init__()
        self.shape = shape

    def isEmpty(self):
        """ This is part of the virtual interface of QLayoutItem.
        """
        return True


class ShapeLayoutItem(QLayout):
    """ A QLayout for holding a single shape object.

    This class exists because Qt apparently doesn't support QLayoutItem
    subclasses. However, since QLayout is a QLayoutItem, you can subclass it to
    get the same behavior when you want to add an item to a layout which isn't
    a layout or a widget (or a spacer). So here we are.
    """
    def __init__(self, shape):
        super(ShapeLayoutItem, self).__init__()
        self._item = None
        self.addItem(_ShapeLayoutItemChild(shape))

    @property
    def model(self):
        return self._item.shape.model

    def draw(self, painter):
        shape = self._item.shape
        if shape.is_visible():
            with save_painter_state(painter):
                shape.draw(painter)

    def translate(self, offset):
        self._item.shape.translate(offset)

    def addItem(self, item):
        """ This is part of the virtual interface of QLayout.
        """
        self._item = item

    def count(self):
        """ This is part of the virtual interface of QLayout.
        """
        return 0 if self._item is None else 1

    def geometry(self):
        """ This is part of the virtual interface of QLayout.
        """
        return self._item.shape.geometry()

    def itemAt(self, index):
        """ This is part of the virtual interface of QLayout.
        """
        if index == 0:
            return self._item

    def minimumSize(self):
        """ This is part of the virtual interface of QLayout.
        """
        return self._item.shape.minimumSize()

    def maximumSize(self):
        """ This is part of the virtual interface of QLayout.
        """
        return self._item.shape.maximumSize()

    def sizeHint(self):
        """ This is part of the virtual interface of QLayout.
        """
        return self.minimumSize()

    def setGeometry(self, rect):
        """ This is part of the virtual interface of QLayout.
        """
        self._item.shape.set_geometry(rect)

    def takeAt(self, index):
        """ This is part of the virtual interface of QLayout.
        """
        if index == 0:
            self._item = None
        return None
