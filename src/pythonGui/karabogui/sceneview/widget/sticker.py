#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on February 18, 2020
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtCore import pyqtSlot, QRect
from PyQt5.QtGui import QColor
from PyQt5.QtWidgets import QAction, QDialog, QPlainTextEdit

from karabogui.dialogs.stickerdialog import GREY, StickerDialog
from karabogui.widgets.hints import KaraboSceneWidget


class StickerWidget(KaraboSceneWidget, QPlainTextEdit):
    """A `StickerWidget` for creating editable text widgets on the `scene`"""

    def __init__(self, model, parent=None):
        super(StickerWidget, self).__init__(model=model, parent=parent)
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
        super(StickerWidget, self).destroy()

    def set_visible(self, visible):
        """Satisfy the informal widget interface."""

    def update_global_access_level(self, level):
        """Satisfy the informal widget interface."""

    def set_geometry(self, rect):
        """Satisfy the informal widget interface."""
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)

    @pyqtSlot()
    def edit_colors_text(self):
        parent = self.parent()
        self.edit(parent)

    def edit(self, scene_view):
        dialog = StickerDialog(self.model, parent=scene_view)
        if dialog.exec_() == QDialog.Rejected:
            return
        model = dialog.model
        self.model.trait_set(text=model.text,
                             font=model.font, background=model.background,
                             foreground=model.foreground)
        self.set_widget_properties(model)

    def set_widget_properties(self, model):
        """Set the new widget properties according to the model"""
        self.setPlainText(model.text)
        sheet = []
        sheet.append('qproperty-font: "{}";'.format(model.font))
        sheet.append('color: "{}";'.format(model.foreground))
        sheet.append('background-color: "{}";'.format(model.background))

        # Add borders
        bg = model.background
        if bg == 'transparent':
            bg = GREY
        color = QColor(bg).darker(120).name()
        sheet.append('border: 1px solid;')
        sheet.append('border-top: 5px solid;')
        sheet.append('border-color: {};'.format(color))

        self.setStyleSheet("QPlainTextEdit {{ {} }}".format("".join(
            sheet)))
