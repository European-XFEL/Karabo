#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtCore import QMargins, pyqtSlot, QSize, Qt
from PyQt5.QtGui import QColor, QFont, QFontMetrics, QPainter, QPen
from PyQt5.QtWidgets import QAction, QDialog, QLabel

from karabogui.dialogs.textdialog import TextDialog
from karabogui.widgets.hints import KaraboSceneWidget

from .utils import get_size_from_dpi

MARGINS = (5, 0, 5, 0)  # left, top, right, bottom
WIDTH_MARGIN = MARGINS[0] + MARGINS[2]
HEIGHT_MARGIN = MARGINS[1] + MARGINS[3]


class LabelWidget(KaraboSceneWidget, QLabel):
    """A label which can appear in a scene
    """

    def __init__(self, model, parent=None):
        super(LabelWidget, self).__init__(model.text,
                                          model=model, parent=parent)
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
        """Reimplemented `sizeHint` of KaraboSceneWidget since we want to get
        the suggested sizeHint from the font metrics and not from `QLabel`

        Calculate the size hint from the text if model is newly
        instantiated (no width/height yet). Else, return the model size."""
        size = QSize()
        model = self.model
        if (model.x, model.y) != (0, 0):
            size = QSize(model.width, model.height)
        if size.isEmpty():
            fm = QFontMetrics(self.font())
            width = fm.width(self.model.text)
            height = fm.height() + self.model.frame_width
            size = QSize(width + WIDTH_MARGIN, max(height, 20) + HEIGHT_MARGIN)
            # Calculate effective size from the DPI of the OS. This is needed
            # as MacOSX DPI has a different value from the rest.
            size = get_size_from_dpi(size)
        return size

    def paintEvent(self, event):
        with QPainter(self) as painter:
            # Calculate the effective rectangle which considers the
            # frame width as it is expands inwards. The bottom and the right
            # edge needs to have another pixel offset.
            eff_rect = self.model.frame_width / 2
            boundary = self.rect()
            painter.fillRect(boundary, QColor(self.model.background))
            fore_color = QColor(self.model.foreground)
            # Draw the boundary and adjust the rect
            if self.model.frame_width:
                boundary = boundary.adjusted(eff_rect, eff_rect,
                                             -eff_rect - 1, -eff_rect - 1)
                pen = QPen(fore_color)
                pen.setWidth(self.model.frame_width)
                painter.setPen(pen)
                painter.drawRect(boundary)

            # Draw text
            pen = QPen(fore_color)
            painter.setPen(pen)
            painter.setFont(self.font())
            painter.drawText(boundary.marginsRemoved(QMargins(*MARGINS)),
                             self.alignment(), self._get_elided_text())

    @property
    def text_width(self):
        return self.width() - WIDTH_MARGIN

    def _get_elided_text(self):
        fm = QFontMetrics(self.font())
        text = self.model.text
        text_width = self.text_width
        if fm.width(text) > text_width:
            text = fm.elidedText(text, Qt.ElideRight, text_width)

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
