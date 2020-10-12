#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtCore import pyqtSlot, QSize, Qt
from PyQt5.QtGui import QColor, QFont, QFontMetrics, QPainter, QPen
from PyQt5.QtWidgets import QAction, QDialog, QLabel

from karabogui.dialogs.textdialog import TextDialog
from karabogui.fonts import substitute_font


class LabelWidget(QLabel):
    """A label which can appear in a scene
    """
    def __init__(self, model, parent=None):
        super(LabelWidget, self).__init__(model.text, parent)
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

        qfont = QFont()
        qfont.fromString(model.font)
        self.setFont(qfont)

        self.setToolTip(model.text)
        self.setGeometry(model.x, model.y, model.width, model.height)

    def sizeHint(self):
        """Calculate the size hint from the text if model is newly
           instantiated (no width/height yet). Else, return the model size."""
        if self.model.width == 0 and self.model.height == 0:
            # Calculate the suggested widget size from the model text
            fm = QFontMetrics(self.font())
            CONTENT_MARGIN = 10
            width = fm.width(self.model.text) + CONTENT_MARGIN
            height = fm.height() + self.model.frame_width
            return QSize(width, max(height, 20))
        return QSize(self.model.width, self.model.height)

    def minimumSizeHint(self):
        return QSize(self.model.width, self.model.height)

    def paintEvent(self, event):
        with QPainter(self) as painter:
            # Calculate the effective rectangle which considers the
            # frame width as it is expands inwards. The bottom and the right
            # edge needs to have another pixel offset.
            eff_rect = self.model.frame_width / 2
            boundary = self.rect().adjusted(eff_rect, eff_rect,
                                            -eff_rect - 1, -eff_rect - 1)
            painter.fillRect(boundary, QColor(self.model.background))

            # Draw the boundary
            if self.model.frame_width:
                pen = QPen(Qt.black)
                pen.setWidth(self.model.frame_width)
                painter.setPen(pen)
                painter.drawRect(self.rect())

            # Draw text
            pen = QPen(QColor(self.model.foreground))
            painter.setPen(pen)
            painter.setFont(self.font())
            painter.drawText(boundary, self.alignment(),
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
