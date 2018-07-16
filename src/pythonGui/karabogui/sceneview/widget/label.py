#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
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
        self.set_model(model)

    def set_model(self, model):
        """Set the new ``model`` and update the widget properties.
        """
        self.model = model

        self.setText(self.model.text)
        self.setToolTip(self.model.text)
        self.setLineWidth(self.model.frame_width)

        font_properties = QFont()
        font_properties.fromString(self.model.font)
        self.setFont(font_properties)

        palette = self.palette()
        palette.setColor(self.foregroundRole(), QColor(model.foreground))
        palette.setColor(self.backgroundRole(), QColor(model.background))
        self.setPalette(palette)

        _, _, model.width, model.height = calc_rect_from_text(model.font,
                                                              model.text)
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

    def add_custom_action(self, main_menu):
        """This method is the handler which will be triggered when the user do
        a right click on the widget.

        :param menu_action: the QMenuAction to manage the menu
        """
        edit_label = QAction("Edit Label", self)
        edit_label.triggered.connect(self.edit)
        main_menu.addAction(edit_label)
        main_menu.addSeparator()

    def edit(self, scene_view):
        dialog = TextDialog(self.model)
        if dialog.exec() == QDialog.Rejected:
            return

        self.set_model(dialog.label_model)
