#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtCore import pyqtSlot, QSize, Qt
from PyQt5.QtGui import QFontMetrics, QPainter
from PyQt5.QtWidgets import (
    QAction, QDialog, QFrame, QLabel, QStyle, QStyleOption)

from karabogui.dialogs.textdialog import TextDialog
from karabogui.fonts import substitute_font
from karabogui.util import generateObjectName


class LabelWidget(QLabel):
    """A label which can appear in a scene
    """
    def __init__(self, model, parent=None):
        super(LabelWidget, self).__init__(model.text, parent)
        self.setFrameShape(QFrame.Box)
        self.setAutoFillBackground(True)
        self.setObjectName(generateObjectName(self))
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

        self.setToolTip(model.text)
        self.setLineWidth(model.frame_width)
        styleSheet = []
        styleSheet.append('qproperty-font: "{}";'.format(model.font))
        styleSheet.append('color: "{}";'.format(model.foreground))
        if model.background:
            styleSheet.append('background-color: "{}";'.format(
                model.background))
        self.setStyleSheet("QLabel#{name} {{ {stylesheet} }}".format(
            name=self.objectName(), stylesheet="".join(styleSheet)))
        self.setGeometry(model.x, model.y, model.width, model.height)

    def sizeHint(self):
        """Calculate the size hint from the text if model is newly
           instantiated (no width/height yet). Else, return the model size."""
        if self.model.width == 0 and self.model.height == 0:
            # Calculate the suggested widget size from the model text
            fm = QFontMetrics(self.font())
            CONTENT_MARGIN = 10
            width = fm.width(self.model.text) + CONTENT_MARGIN
            return QSize(width, max(self.height(), 20))
        return QSize(self.model.width, self.model.height)

    def minimumSizeHint(self):
        return QSize(self.model.width, self.model.height)

    def paintEvent(self, event):
        option = QStyleOption()
        with QPainter(self) as painter:
            self.style().drawPrimitive(QStyle.PE_Widget, option, painter, self)
            painter.drawText(self.rect(), self.alignment(),
                             self._get_elided_text())

    def _get_elided_text(self):
        fm = QFontMetrics(self.font())
        text = self.model.text
        if fm.width(text) > self.width():
            text = fm.elidedText(text, Qt.ElideRight, self.width())

        return text

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
