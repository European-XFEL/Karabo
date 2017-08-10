#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 8, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QSize
from PyQt4.QtGui import QStyledItemDelegate

from karabo.middlelayer import AccessMode
from karabo_gui.widget import EditableWidget
from .edit_widget import EditWidgetWrapper

FIXED_ROW_HEIGHT = 30


class ValueDelegate(QStyledItemDelegate):
    """A QStyledItemDelegate for configurator values
    """
    def createEditor(self, parent, option, index):
        """Reimplemented function of QStyledItemDelegate.
        """
        model = index.model()
        box = model.box_ref(index)
        if box is None:
            return None

        descriptor = box.descriptor
        if descriptor is None:
            return None

        is_class = box.configuration.type in ('class', 'projectClass')
        is_class_editable = (is_class and descriptor.accessMode in
                             (AccessMode.INITONLY, AccessMode.RECONFIGURABLE))
        is_inst_editable = (not is_class and
                            descriptor.accessMode is AccessMode.RECONFIGURABLE)

        widget = EditWidgetWrapper(box, parent=parent)
        widget.create_editable_widget(EditableWidget.getClass(box))
        if is_class_editable:
            widget.make_class_connections()
        elif is_inst_editable:
            widget.make_device_connections()
        return widget

    def setModelData(self, editor, model, index):
        """Reimplemented function of QStyledItemDelegate.

        NOTE: This is setting the value directly on the box instead of going
        through the item model.
        """
        model = index.model()
        box = model.box_ref(index)
        if box is None:
            return

        box.signalUserChanged.emit(box, editor.editable_widget.value, None)
        if box.configuration.type == "macro":
            box.set(editor.value)
        elif box.descriptor is not None:
            box.configuration.setUserValue(box, editor.editable_widget.value)
            box.configuration.sendUserValue(box)

    def sizeHint(self, option, index):
        """Reimplemented function of QStyledItemDelegate.

        XXX: I don't like this, but it's not clear how I would access the
        editor widget for a row (_if_ it exists) to return the correct height
        here.
        """
        return QSize(option.rect.width(), FIXED_ROW_HEIGHT)
