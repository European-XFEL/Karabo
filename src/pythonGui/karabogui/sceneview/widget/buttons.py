from qtpy.QtCore import QRect, Qt, Slot
from qtpy.QtWidgets import QAction, QDialog, QInputDialog, QToolButton

from karabogui import icons
from karabogui.dialogs.api import PopupButtonDialog
from karabogui.util import move_to_cursor
from karabogui.widgets.api import TextPopupWidget
from karabogui.widgets.hints import KaraboSceneWidget


class PopupButtonWidget(KaraboSceneWidget, QToolButton):
    """A button that shows a title and detailed text as popup on
    mouse click"""
    def __init__(self, model, parent=None):
        super().__init__(model=model, parent=parent)
        self.setIcon(icons.about)
        self.setMinimumSize(self.iconSize())

        edit_text_action = QAction("Edit Text", self)
        edit_text_action.triggered.connect(self._edit_text)
        self.addAction(edit_text_action)

        edit_label_action = QAction("Edit Label", self)
        edit_label_action.triggered.connect(self._edit_label)
        self.addAction(edit_label_action)

        self.set_widget_properties(model)
        self.setGeometry(QRect(model.x, model.y,
                               model.width, model.height))
        self.setStyleSheet("QToolButton { border: none; }"
                           "QToolButton::menu-indicator {image:none;}")

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

    def set_widget_properties(self, model):
        self.model.text = model.text
        self.model.label = model.label
        tooltip = "Click for details"
        if self.model.label:
            tooltip = f"{self.model.label}... {tooltip}"
            self.setText(self.model.label)
            self.setToolButtonStyle(Qt.ToolButtonTextBesideIcon)
        self.setToolTip(tooltip)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            widget = TextPopupWidget(self.model, parent=self)
            move_to_cursor(widget)
            widget.show()
            event.accept()

    def resizeEvent(self, event):
        """Override to set the icon size to match the widget size."""
        super().resizeEvent(event)
        self.setIconSize(self.size())

    def edit(self, scene_view):
        """Override to open a dialog for editing the sticker."""
        dialog = PopupButtonDialog(self.model, parent=scene_view)
        move_to_cursor(dialog)
        if dialog.exec() == QDialog.Rejected:
            return

        model = dialog.model
        self.model.trait_set(text=model.text,
                             popup_width=model.popup_width,
                             popup_height=model.popup_height)
        self.set_widget_properties(model)

    @Slot()
    def _edit_text(self):
        self.edit(scene_view=self.parent())

    @Slot()
    def _edit_label(self):
        label, ok = QInputDialog.getText(
            self, "Edit Label", "Edit Label:", text=self.model.label)
        if ok:
            self.model.trait_set(label=label)
            self.set_widget_properties(self.model)
