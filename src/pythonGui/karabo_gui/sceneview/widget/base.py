from PyQt4.QtCore import QRect
from PyQt4.QtGui import QStackedLayout, QWidget

from karabo_gui.topology import getDevice


def _get_boxes(model):
    """ Grab all the Box objects for a given widget model object.
    """
    boxes = []
    for k in model.keys:
        deviceId, path = k.split('.', 1)
        conf = getDevice(deviceId)
        boxes.append(conf.getBox(path.split(".")))
    return boxes


class BaseWidgetContainer(QWidget):
    """ A simple container for scene widgets which provides the interface
    needed by the scene tools.
    """
    def __init__(self, model, parent):
        super(BaseWidgetContainer, self).__init__(parent)
        self.model = model
        self.layout = QStackedLayout(self)
        self.layout.setStackingMode(QStackedLayout.StackAll)
        widget = self._create_widget(_get_boxes(model))
        self.layout.addWidget(widget)
        self.setGeometry(QRect(model.x, model.y, model.width, model.height))

    def _create_widget(self, boxes):
        """ A method for creating the child widget.

        THIS MUST BE IMPLEMENTED BY DERIVED CLASSES
        """
        raise NotImplementedError

    def set_geometry(self, rect):
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)
