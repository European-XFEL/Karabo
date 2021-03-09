#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtCore import pyqtSlot, QSize, Qt
from PyQt5.QtGui import QFontMetrics
from PyQt5.QtWidgets import QAction, QDialog, QFrame, QLabel

from karabogui.fonts import get_qfont
from karabogui.dialogs.textdialog import TextDialog
from karabogui.widgets.hints import KaraboSceneWidget

MARGINS = (5, 0, 5, 0)  # left, top, right, bottom
WIDTH_MARGIN = MARGINS[0] + MARGINS[2]
HEIGHT_MARGIN = MARGINS[1] + MARGINS[3]


class LabelWidget(KaraboSceneWidget, QLabel):
    """A label which can appear in a scene
    """

    def __init__(self, model, parent=None):
        super(LabelWidget, self).__init__(model.text,
                                          model=model, parent=parent)
        self.setFrameShape(QFrame.Box)
        self.fm = None
        self.set_model(model)

        # Base edit action
        edit_action = QAction("Edit Label", self)
        edit_action.triggered.connect(self.edit_colors_text)
        self.addAction(edit_action)

    def set_model(self, model):
        """Set the new ``model`` and update the widget properties.
        """
        self.model.trait_set(text=model.text, frame_width=model.frame_width,
                             font=model.font, background=model.background,
                             foreground=model.foreground)
        font = get_qfont(model.font)
        self.setFont(font)
        self.fm = QFontMetrics(font)
        self.setLineWidth(model.frame_width)
        self.setToolTip(model.text)
        self.create_display_text()

        # Set the stylesheet for background and foreground color
        sheet = []
        sheet.append('color: "{}";'.format(model.foreground))
        if model.background:
            sheet.append('background-color: "{}";'.format(
                model.background))
        self.setStyleSheet("QLabel {{ {} }}".format("".join(sheet)))
        self.setGeometry(model.x, model.y, model.width, model.height)

    def minimumSizeHint(self):
        """Reimplemented `sizeHint` of KaraboSceneWidget"""
        fm = QFontMetrics(self.font())
        width = fm.width(self.model.text)
        height = fm.height() + self.model.frame_width
        size = QSize(width + WIDTH_MARGIN, max(height, 20) + HEIGHT_MARGIN)

        return size

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
        return size

    def resizeEvent(self, event):
        """Reimplemented function of PyQt for adjust the display text"""
        super(LabelWidget, self).resizeEvent(event)
        self.create_display_text()

    @property
    def text_width(self):
        return self.width() - WIDTH_MARGIN

    def create_display_text(self):
        """Calculate the effective display text

        Use the font metric to calculate the width of the text. Is the width
        is larger than the widget size, the text is returned as elided.
        For synchronization, this method is invoked on resizeEvents as well.
        """
        display = self.text()
        text = self.model.text
        text_width = self.text_width
        if self.fm.width(text) > text_width:
            text = self.fm.elidedText(text, Qt.ElideRight, text_width)
        if display != text:
            self.setText(text)

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
        self.model.trait_set(x=new_pos.x(), y=new_pos.y())
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
