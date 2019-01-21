#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 23, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import webbrowser

from PyQt4.QtCore import pyqtSlot, QPoint, QRect, QSize, Qt
from PyQt4.QtGui import (
    QAction, QColor, QDialog, QFont, QPainter, QPen, QPushButton)

from karabogui.dialogs.dialogs import SceneLinkDialog
from karabogui.dialogs.textdialog import TextDialog
from karabogui.dialogs.webdialog import WebDialog
from karabogui.events import broadcast_event, KaraboEventSender
from karabogui import messagebox


class SceneLinkWidget(QPushButton):
    """A clickable link which opens another scene
    """

    def __init__(self, model, parent=None):
        super(SceneLinkWidget, self).__init__(parent)
        self.model = model
        self.setToolTip(self.model.target)
        self.setCursor(Qt.PointingHandCursor)
        self.clicked.connect(self._handle_click)
        self.setGeometry(QRect(model.x, model.y, model.width, model.height))

        edit_action = QAction('Edit Label', self)
        edit_action.triggered.connect(self.edit_label)
        self.addAction(edit_action)

        target_action = QAction('Edit Target', self)
        target_action.triggered.connect(self.edit_target)
        self.addAction(target_action)

    def paintEvent(self, event):
        with QPainter(self) as painter:
            boundary = self.rect().adjusted(2, 2, -2, -2)
            pt = boundary.topLeft()
            rects = [QRect(pt, QSize(7, 7)),
                     QRect(pt + QPoint(11, 0), QSize(7, 7))]
            painter.fillRect(boundary, QColor(self.model.background))
            pen = QPen(Qt.black)
            pen.setWidth(self.model.frame_width)
            painter.setPen(pen)
            painter.drawRect(boundary)
            pen.setColor(Qt.darkGray)
            pen.setWidth(3)
            painter.setPen(pen)
            painter.drawRects(rects)
            pen.setColor(Qt.lightGray)
            painter.setPen(pen)
            painter.drawLine(pt + QPoint(4, 4), pt + QPoint(15, 4))
            # Before painting the text, set the font
            font_properties = QFont()
            font_properties.fromString(self.model.font)
            painter.setFont(font_properties)
            pen = QPen(QColor(self.model.foreground))
            painter.setPen(pen)
            painter.drawText(boundary, Qt.AlignCenter, self.model.text)

    @pyqtSlot()
    def _handle_click(self):
        if len(self.model.target) > 0:
            parts = self.model.target.split(':')
            if len(parts) != 2:
                return

            # target format => "simple_name:UUID"
            name = parts[0]
            target = parts[1]
            target_window = self.model.target_window
            # Broadcast an event
            data = {'name': name, 'target': target,
                    'target_window': target_window}
            broadcast_event(KaraboEventSender.OpenSceneLink, data)

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
    def edit_label(self):
        dialog = TextDialog(self.model)
        if dialog.exec() == QDialog.Rejected:
            return

        # Set all at once!
        label = dialog.label_model
        self.model.trait_set(text=label.text, frame_width=label.frame_width,
                             font=label.font, background=label.background,
                             foreground=label.foreground)

    @pyqtSlot()
    def edit_target(self):
        self.edit()

    def edit(self, scene_view=None):
        dialog = SceneLinkDialog(self.model, parent=scene_view)
        if dialog.exec() == QDialog.Rejected:
            return

        self.model.target = dialog.selectedScene
        self.model.target_window = dialog.selectedTargetWindow
        self.setToolTip(self.model.target)


class WebLinkWidget(QPushButton):
    """A clickable widget which opens a hyperlink
    """

    def __init__(self, model, parent=None):
        super(WebLinkWidget, self).__init__(parent)
        self.model = model
        self.setToolTip(self.model.target)
        self.setCursor(Qt.PointingHandCursor)
        self.clicked.connect(self._handle_click)
        self.setGeometry(QRect(model.x, model.y, model.width, model.height))

        edit_action = QAction('Edit Label', self)
        edit_action.triggered.connect(self.edit_label)
        self.addAction(edit_action)

        target_action = QAction('Edit Target', self)
        target_action.triggered.connect(self.edit_target)
        self.addAction(target_action)

    def paintEvent(self, event):
        with QPainter(self) as painter:
            boundary = self.rect().adjusted(2, 2, -2, -2)
            pt = boundary.topLeft()
            rects = [QRect(pt, QSize(7, 7)),
                     QRect(pt + QPoint(11, 0), QSize(7, 7))]
            painter.fillRect(boundary, QColor(self.model.background))
            pen = QPen(Qt.black)
            pen.setWidth(self.model.frame_width)
            painter.setPen(pen)
            painter.drawRect(boundary)
            pen.setColor(QColor(255, 145, 255))
            pen.setWidth(3)
            painter.setPen(pen)
            painter.drawRects(rects)
            pen.setColor(Qt.lightGray)
            painter.setPen(pen)
            painter.drawLine(pt + QPoint(4, 4), pt + QPoint(15, 4))
            # Before painting the text, set the font
            font_properties = QFont()
            font_properties.fromString(self.model.font)
            painter.setFont(font_properties)
            pen = QPen(QColor(self.model.foreground))
            painter.setPen(pen)
            painter.drawText(boundary, Qt.AlignCenter, self.model.text)

    @pyqtSlot()
    def _handle_click(self):
        if len(self.model.target) > 0:
            try:
                webbrowser.open_new(self.model.target)
            except webbrowser.Error:
                messagebox.show_error("No web browser available!")

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
    def edit_label(self):
        dialog = TextDialog(self.model)
        if dialog.exec() == QDialog.Rejected:
            return

        # Set all at once!
        label = dialog.label_model
        self.model.trait_set(text=label.text, frame_width=label.frame_width,
                             font=label.font, background=label.background,
                             foreground=label.foreground)

    @pyqtSlot()
    def edit_target(self):
        self.edit()

    def edit(self, scene_view=None):
        dialog = WebDialog(self.model.target)
        if dialog.exec() == QDialog.Rejected:
            return

        self.model.target = dialog.target
        self.setToolTip(self.model.target)
