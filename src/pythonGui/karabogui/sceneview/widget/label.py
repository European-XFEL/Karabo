#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
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
from qtpy.QtCore import QSize, Qt, Slot
from qtpy.QtGui import QFontMetrics
from qtpy.QtWidgets import QAction, QDialog, QFrame, QLabel, QSizePolicy

from karabogui.dialogs.textdialog import TextDialog
from karabogui.fonts import get_qfont
from karabogui.widgets.hints import KaraboSceneWidget

MARGINS = (5, 0, 5, 0)  # left, top, right, bottom
WIDTH_MARGIN = MARGINS[0] + MARGINS[2]
HEIGHT_MARGIN = MARGINS[1] + MARGINS[3]


class LabelWidget(KaraboSceneWidget, QLabel):
    """A label which can appear in a scene
    """

    def __init__(self, model, parent=None):
        super().__init__(model.text, model=model, parent=parent)
        self.setSizePolicy(QSizePolicy.MinimumExpanding,
                           QSizePolicy.Minimum)
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
                             foreground=model.foreground,
                             alignh=model.alignh)
        font = get_qfont(model.font)
        self.setFont(font)
        self.setAlignment(Qt.AlignmentFlag(model.alignh) | Qt.AlignVCenter)
        self.fm = QFontMetrics(font)
        self.setLineWidth(model.frame_width)
        self.setToolTip(model.text)

        # Set the stylesheet for background and foreground color
        sheet = []
        sheet.append(f"color: {model.foreground};")
        if model.background:
            sheet.append(f"background-color: {model.background};")
        self.setStyleSheet(f"QLabel {{ {''.join(sheet)} }}")
        self.setGeometry(model.x, model.y, model.width, model.height)
        self.create_display_text()

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
        super().resizeEvent(event)
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
        super().destroy()

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

    @Slot()
    def edit_colors_text(self):
        self.edit(scene_view=None)
        self.update()

    def edit(self, scene_view):
        dialog = TextDialog(self.model, alignment=True, parent=scene_view)
        if dialog.exec() == QDialog.Rejected:
            return

        self.set_model(dialog.label_model)
