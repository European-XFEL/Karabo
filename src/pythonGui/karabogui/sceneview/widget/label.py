#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QAction, QColor, QDialog, QFont, QFrame, QLabel

from karabogui.dialogs.textdialog import TextDialog
from karabogui.sceneview.utils import calc_rect_from_text


class LabelWidget(QLabel):
    """A label which can appear in a scene
    """

    def __init__(self, model, parent=None):
        super(LabelWidget, self).__init__(model.text, parent)
        self.setFrameShape(QFrame.Box)
        self.setAutoFillBackground(True)
        self.model = model
        self.apply_model()
        edit_action = QAction("Edit Label", self)
        edit_action.triggered.connect(self.edit_colors_text)
        self.addAction(edit_action)

    def set_model(self, model):
        self.model.trait_set(text=model.text, frame_width=model.frame_width,
                             font=model.font, background=model.background,
                             foreground=model.foreground)
        self.apply_model()

    def apply_model(self):
        model = self.model
        self.setText(model.text)
        self.setToolTip(model.text)
        self.setLineWidth(model.frame_width)

        font_properties = QFont()
        font_properties.fromString(model.font)
        self.setFont(font_properties)

        palette = self.palette()
        palette.setColor(self.foregroundRole(), QColor(model.foreground))
        palette.setColor(self.backgroundRole(), QColor(model.background))
        self.setPalette(palette)
        _, _, model.width, model.height = calc_rect_from_text(
            model.font, model.text)
        self.setGeometry(model.x, model.y, model.width, model.height)

    def add_proxies(self, proxies):
        """Satisfy the informal widget interface."""

    def apply_changes(self):
        """Satisfy the informal widget interface."""

    def decline_changes(self):
        """Satisfy the informal widget interface."""

    def destroy(self):
        """Satisfy the informal widget interface."""

    def set_visible(self, visible):
        """Satisfy the informal widget interface."""

    def update_alarm(self):
        """Satisfy the informal widget interface."""

    def update_global_access_level(self, level):
        """Satisfy the informal widget interface."""

    def set_geometry(self, rect):
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)

    @pyqtSlot()
    def edit_colors_text(self):
        self.edit(scene_view=None)
        self.update()

    def edit(self, scene_view):
        dialog = TextDialog(self.model)
        if dialog.exec() == QDialog.Rejected:
            return

        self.set_model(dialog.label_model)
