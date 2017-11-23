#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QDialog, QFrame, QLabel

from karabogui.dialogs.textdialog import TextDialog
from karabogui.sceneview.utils import calc_rect_from_text


class LabelWidget(QLabel):
    """A label which can appear in a scene
    """
    def __init__(self, model, parent=None):
        super(LabelWidget, self).__init__(model.text, parent)
        self.setFrameShape(QFrame.Box)
        self.set_model(model)

    def set_model(self, model):
        """Set the new ``model`` and update the widget properties.
        """
        self.model = model

        self.setText(self.model.text)
        self.setToolTip(self.model.text)
        self.setLineWidth(model.frame_width)

        styleSheet = []
        styleSheet.append('qproperty-font: "{}";'.format(model.font))
        styleSheet.append('color: "{}";'.format(model.foreground))
        if model.background:
            styleSheet.append('background-color: "{}";'.format(
                model.background))
        self.setStyleSheet("".join(styleSheet))

        _, _, model.width, model.height = calc_rect_from_text(model.font,
                                                              model.text)
        self.setGeometry(model.x, model.y, model.width, model.height)

    def add_proxies(self, proxies):
        """Satisfy the informal widget interface."""

    def apply_changes(self, devices=None, send_immediately=False):
        """Satisfy the informal widget interface."""

    def decline_changes(self):
        """Satisfy the informal widget interface."""

    def destroy(self):
        """Satisfy the informal widget interface."""

    def set_visible(self, visible):
        """Satisfy the informal widget interface."""

    def update_alarm_symbol(self):
        """Satisfy the informal widget interface."""

    def update_global_access_level(self):
        """Satisfy the informal widget interface."""

    def set_geometry(self, rect):
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)

    def edit(self, scene_view):
        dialog = TextDialog(self.model)
        if dialog.exec() == QDialog.Rejected:
            return

        self.set_model(dialog.label_model)
