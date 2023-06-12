#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on February 18, 2020
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
#############################################################################
from qtpy.QtCore import QRect, Slot
from qtpy.QtGui import QColor
from qtpy.QtWidgets import QAction, QDialog, QPlainTextEdit

from karabogui.dialogs.api import GREY, StickerDialog
from karabogui.fonts import get_qfont
from karabogui.widgets.hints import KaraboSceneWidget


class StickerWidget(KaraboSceneWidget, QPlainTextEdit):
    """A `StickerWidget` for creating editable text widgets on the `scene`"""

    def __init__(self, model, parent=None):
        super().__init__(model=model, parent=parent)
        self.set_widget_properties(model)
        self.setGeometry(QRect(model.x, model.y, model.width, model.height))
        self.setReadOnly(True)

        # Enforce the same frame style as with the dialog text edit to
        # have the same scroll area dimensions
        self.setFrameShape(QPlainTextEdit.Box | QPlainTextEdit.Plain)
        edit_action = QAction("Edit Sticker", self)
        edit_action.triggered.connect(self.edit_colors_text)
        self.addAction(edit_action)

    def add_proxies(self, proxies):
        """Satisfy the informal widget interface."""

    def apply_changes(self):
        """Satisfy the informal widget interface."""

    def decline_changes(self):
        """Satisfy the informal widget interface."""

    def destroy(self):
        """Satisfy the informal widget interface."""
        super().destroy()

    def set_visible(self, visible):
        """Satisfy the informal widget interface."""

    def update_global_access_level(self, level):
        """Satisfy the informal widget interface."""

    def set_geometry(self, rect):
        """Satisfy the informal widget interface."""
        self.model.trait_set(x=rect.x(), y=rect.y(),
                             width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.trait_set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)

    @Slot()
    def edit_colors_text(self):
        parent = self.parent()
        self.edit(parent)

    def edit(self, scene_view):
        dialog = StickerDialog(self.model, parent=scene_view)
        if dialog.exec() == QDialog.Rejected:
            return
        model = dialog.model
        self.model.trait_set(text=model.text,
                             font=model.font, background=model.background,
                             foreground=model.foreground)
        self.set_widget_properties(model)

    def set_widget_properties(self, model):
        """Set the new widget properties according to the model"""
        self.setPlainText(model.text)

        # We have to use the effective font string to correct font size
        font_string = get_qfont(model.font).toString()

        sheet = []
        sheet.append(f'qproperty-font: "{font_string}";')
        sheet.append(f'color: "{model.foreground}";')
        sheet.append(f'background-color: "{model.background}";')

        # Add borders
        bg = model.background
        if bg == 'transparent':
            bg = GREY
        color = QColor(bg).darker(120).name()
        sheet.append('border: 1px solid;')
        sheet.append('border-top: 5px solid;')
        sheet.append(f'border-color: {color};')

        self.setStyleSheet("QPlainTextEdit {{ {} }}".format("".join(
            sheet)))
