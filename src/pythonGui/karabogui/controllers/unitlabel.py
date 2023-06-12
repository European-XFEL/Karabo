# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from qtpy.QtWidgets import QFrame, QHBoxLayout, QLabel, QSizePolicy


class UnitLabelWrapper(QFrame):
    """NOTE: We are not deriving a class from QWidget here, because doing so
    breaks the usage of stylesheets elsewhere in the GUI (configurator).
    The `update_label` function is attached to the returned widget as a
    workaround...
    """

    def __init__(self, widget, parent=None):
        super().__init__(parent)

        # Store internal widget reference
        self._internal_widget = widget

        # Add label
        self.label = label = QLabel(self)
        label.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Preferred)

        # Add and populate layout
        layout = QHBoxLayout(self)
        layout.setSizeConstraint(QHBoxLayout.SetNoConstraint)
        layout.setSpacing(0)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(widget)
        layout.addWidget(label)

    def update_unit_label(self, proxy):
        """Update the label according to the unit_label of the `proxy`"""
        if proxy.binding is None:
            return

        unit_label = proxy.binding.unit_label
        self.label.setVisible(unit_label != "")
        self.label.setText(unit_label)

    def update_label(self, proxy):
        # Add an `update_label` "method" for keeping things synced
        if proxy.binding is None:
            return

        unit_label = proxy.binding.unit_label
        if unit_label != self.label.text():
            self.label.setText(unit_label)

    def setFont(self, font):
        super().setFont(font)
        self._internal_widget.setFont(font)
        self.label.setFont(font)


def add_unit_label(proxy, widget, parent=None):
    """Add a unit label to a widget, if available."""

    wrapper_widget = UnitLabelWrapper(widget, parent=parent)
    wrapper_widget.update_label(proxy)
    return wrapper_widget
