#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtCore import pyqtSlot, QSize
from PyQt5.QtGui import QFontMetrics
from PyQt5.QtWidgets import QAction, QDialog, QFrame, QLabel

from karabogui.dialogs.textdialog import TextDialog
from karabogui.fonts import substitute_font


class LabelWidget(QLabel):
    """A label which can appear in a scene
    """
    def __init__(self, model, parent=None):
        super(LabelWidget, self).__init__(model.text, parent)
        self.setFrameShape(QFrame.Box)
        self.setAutoFillBackground(True)
        self.model = model
        # Check and substitute the font with the application fonts
        substitute_font(model)
        self.set_model(model)
        edit_action = QAction("Edit Label", self)
        edit_action.triggered.connect(self.edit_colors_text)
        self.addAction(edit_action)

    def set_model(self, model):
        """Set the new ``model`` and update the widget properties.
        """
        self.model.trait_set(text=model.text, frame_width=model.frame_width,
                             font=model.font, background=model.background,
                             foreground=model.foreground)

        self.setText(model.text)
        self.setToolTip(model.text)
        self.setLineWidth(model.frame_width)
        styleSheet = []
        styleSheet.append('qproperty-font: "{}";'.format(model.font))
        styleSheet.append('color: "{}";'.format(model.foreground))
        if model.background:
            styleSheet.append('background-color: "{}";'.format(
                model.background))
        self.setStyleSheet("QLabel {{ {} }}".format("".join(styleSheet)))
        self.setGeometry(model.x, model.y, model.width, model.height)

    def sizeHint(self):
        fm = QFontMetrics(self.font())
        CONTENT_MARGIN = 10
        width = fm.width(self.text()) + CONTENT_MARGIN

        return QSize(width, 20)

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

    def update_global_access_level(self, level):
        """Satisfy the informal widget interface."""

    def set_geometry(self, rect):
        self.model.trait_set(x=rect.x(), y=rect.y(),
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
        dialog = TextDialog(self.model, parent=scene_view)
        if dialog.exec_() == QDialog.Rejected:
            return

        self.set_model(dialog.label_model)
