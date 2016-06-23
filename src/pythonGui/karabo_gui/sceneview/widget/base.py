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
        boxes = _get_boxes(model)
        self.old_style_widget = self._create_widget(boxes)
        self.box = boxes[0]
        self.layout.addWidget(self.old_style_widget.widget)
        self._make_box_connections()
        self.setGeometry(QRect(model.x, model.y, model.width, model.height))

    def _create_widget(self, boxes):
        """ A method for creating the child widget.

        THIS MUST BE IMPLEMENTED BY DERIVED CLASSES
        """
        raise NotImplementedError

    def _make_box_connections(self):
        """ Hook up all the box signals to the old_style_widget instance.
        """
        box = self.box
        widget = self.old_style_widget
        box.signalNewDescriptor.connect(widget.typeChangedSlot)
        if box.descriptor is not None:
            widget.typeChangedSlot(box)
        box.signalUpdateComponent.connect(widget.valueChangedSlot)
        if box.hasValue():
            widget.valueChanged(box, box.value, box.timestamp)

    def destroy(self):
        """ Disconnect the box signals
        """
        box = self.box
        widget = self.old_style_widget
        box.signalNewDescriptor.disconnect(widget.typeChangedSlot)
        box.signalUpdateComponent.disconnect(widget.valueChangedSlot)

    def set_geometry(self, rect):
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)
