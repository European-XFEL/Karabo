#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 14, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from karabo_gui.displaywidgets.displaylabel import DisplayLabel
import karabo_gui.icons as icons
from karabo_gui.util import write_only_property
from .base_item import BaseTreeWidgetItem


class AttributeTreeWidgetItem(BaseTreeWidgetItem):
    def __init__(self, box, attr_name, parent, parentItem=None):
        super(AttributeTreeWidgetItem, self).__init__(box, parent, parentItem)
        self.setIcon(0, icons.signal)
        self.create_display_widget(DisplayLabel, box)
        self._attr_name = attr_name

    def make_class_connections(self, box):
        # Overwrite base class method here
        self.editable_widget.attr_name = self._attr_name
        value = getattr(box.descriptor, self._attr_name)
        self.editable_widget.attributeValueChanged(value)

        self.connect_signal(self.editable_widget.signalEditingFinished,
                            self._on_editing_finished)
        self.connect_signal(box.signalUserChanged,
                            self.editable_widget.valueChanged)

    @write_only_property
    def displayText(self, text):
        self.setText(0, text)
        self.treeWidget().resizeColumnToContents(0)

    def setupContextMenu(self):
        # item specific menu
        # add actions from attributeWidget
        if self.editable_widget is None:
            return None

    def _on_editing_finished(self, box, value):
        setattr(box.descriptor, self._attr_name, value)

        # Configuration changed - so project needs to be informed
        if box.configuration.type == 'projectClass':
            box.configuration.signalBoxChanged.emit()
