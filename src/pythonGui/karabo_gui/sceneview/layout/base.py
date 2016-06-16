from PyQt4.QtCore import QRect
from PyQt4.QtGui import QWidgetItem

from .shape import ShapeLayoutItem


class BaseLayout(object):
    """ This is a mix-in class intended to be inherited by all SceneView
    layouts. It implements generic parts of the QLayout virtual interface.
    """
    def __init__(self, model, *args):
        super(BaseLayout, self).__init__(*args)
        self.model = model

    def __iter__(self):
        """ Implement the Python iterator protocol for all layouts.
        """
        for i in range(self.count()):
            yield self.itemAt(i)

    def _add_layout(self, layout):
        """ Needs to be reimplemented in the inherited classes to add a layout.
        """
        raise NotImplementedError("BaseLayout._add_layout")

    def _add_shape(self, shape):
        """ Use our special hacky workaround for adding shapes to the layout.
        """
        self._add_layout(ShapeLayoutItem(shape))

    def _add_widget(self, widget):
        """ Needs to be reimplemented in the inherited classes to add a widget.
        """
        raise NotImplementedError("BaseLayout._add_widget")

    def _remove_layout(self, layout):
        self.removeItem(layout)

    def _remove_shape(self, shape):
        for item in self:
            if isinstance(item, ShapeLayoutItem) and item.model is shape.model:
                # Call QLayout::removeItem()
                self.removeItem(item)
                return

    def _remove_widget(self, widget):
        self.removeWidget(widget)

    def draw(self, painter):
        for item in self:
            if not isinstance(item, QWidgetItem):
                item.draw(painter)

    def hide(self):
        for item in self:
            if isinstance(item, QWidgetItem):
                item.widget().hide()
            else:
                item.hide()

    def show(self):
        for item in self:
            if isinstance(item, QWidgetItem):
                item.widget().show()
            else:
                item.show()

    def set_geometry(self, rect):
        self.setGeometry(rect)

    def translate(self, offset):
        rect = QRect(self.model.x + offset.x(), self.model.y + offset.y(),
                     self.model.width, self.model.height)
        self.setGeometry(rect)

    # --------------------------------------------
    # QLayout Virtual Functions

    def geometry(self):
        return QRect(self.model.x, self.model.y, self.model.width,
                     self.model.height)

    def setGeometry(self, rect):
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        self.invalidate()  # Important! Force a full layout recalculation
        super(BaseLayout, self).setGeometry(rect)
