from PyQt4.QtCore import QRect, QSize, QTimer, pyqtSlot
from PyQt4.QtGui import (QAction, QHBoxLayout, QStackedLayout, QToolButton,
                         QWidget)

from karabo_gui import icons
from karabo_gui.network import Network
from karabo_gui.topology import getDeviceBox
from .utils import get_box, determine_if_value_unchanged


class BaseWidgetContainer(QWidget):
    """ A container for scene widgets which provides the interface
    needed by the scene tools.
    """
    def __init__(self, model, parent):
        super(BaseWidgetContainer, self).__init__(parent)
        self.model = model
        self.boxes = [get_box(*key.split('.', 1)) for key in self.model.keys]
        self.old_style_widget = self._create_widget(self.boxes)
        for box in self.boxes:
            self._make_box_connections(box)
        if self.model.parent_component == 'EditableApplyLaterComponent':
            self.layout = self._add_edit_widgets()
        else:
            self.layout = QStackedLayout(self)
            self.layout.setStackingMode(QStackedLayout.StackAll)
            self.layout.addWidget(self.old_style_widget.widget)
        self.setGeometry(QRect(model.x, model.y, model.width, model.height))

    def _create_widget(self, boxes):
        """ A method for creating the child widget.

        THIS MUST BE IMPLEMENTED BY DERIVED CLASSES
        """
        raise NotImplementedError

    def destroy(self):
        """ Disconnect the box signals
        """
        from karabo_gui import gui

        widget = self.old_style_widget
        for box in self.boxes:
            box.signalNewDescriptor.disconnect(widget.typeChangedSlot)
            if self.model.parent_component == 'EditableApplyLaterComponent':
                widget.signalEditingFinished.disconnect(self._on_editing_finished)
                box.signalUserChanged.disconnect(self._on_user_edit)
                box.signalUpdateComponent.disconnect(self._on_display_value_change)
                # These are connected in `EditableWidget.__init__`
                box.configuration.boxvalue.state.signalUpdateComponent.disconnect(
                    widget.updateStateSlot)
                gui.window.signalGlobalAccessLevelChanged.disconnect(
                    widget.updateStateSlot)
            else:  # DisplayWidgets
                box.signalUpdateComponent.disconnect(widget.valueChangedSlot)

    def set_visible(self, visible):
        """ Set whether this widget is seen by the user."""
        if visible:
            for box in self.boxes:
                box.addVisible()
        else:
            for box in self.boxes:
                box.removeVisible()

    def set_geometry(self, rect):
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)

    def _make_box_connections(self, box):
        """ Hook up all the box signals to the old_style_widget instance.
        """
        widget = self.old_style_widget
        box.signalNewDescriptor.connect(widget.typeChangedSlot)
        if box.descriptor is not None:
            widget.typeChangedSlot(box)
        box.signalUpdateComponent.connect(widget.valueChangedSlot)
        if box.hasValue():
            widget.valueChanged(box, box.value, box.timestamp)
        if self.model.parent_component == 'EditableApplyLaterComponent':
            widget.signalEditingFinished.connect(self._on_editing_finished)
            box.signalUserChanged.connect(self._on_user_edit)
            box.signalUpdateComponent.disconnect(widget.valueChangedSlot)
            box.signalUpdateComponent.connect(self._on_display_value_change)
            widget.setReadOnly(False)
        else:
            widget.setReadOnly(True)

    # ---------------------------------------------------------------------
    # Qt Methods

    def dropEvent(self, event):
        """ Add another box to a display widget which allows more than one
            boxes. """
        source = event.source()
        if source is None:
            return

        widget = self.old_style_widget
        if self.model.parent_component == 'DisplayComponent':
            for item in source.selectedItems():
                key_list = item.box.key().split('.', 1)
                device_id = key_list[0]
                property_path = key_list[1]
                device_box = get_box(device_id, property_path)
                if widget.addBox(device_box):
                    self.model.keys.append(device_box.key())
                    self.boxes.append(device_box)
                    self._make_box_connections(self.boxes[-1])
                    event.accept()
        super(BaseWidgetContainer, self).dropEvent(event)

    # ---------------------------------------------------------------------
    # Edit buttons related code

    def _add_edit_widgets(self):
        """ Add the extra buttons needed for editable widgets
        """
        def _create_button(text, description, icon, trigger, layout):
            button = QAction(icon, text, self)
            button.setStatusTip(description)
            button.setToolTip(description)
            button.triggered.connect(trigger)
            tb = QToolButton()
            tb.setDefaultAction(button)
            tb.setIconSize(QSize(24, 24))
            layout.addWidget(tb)
            return button, tb

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.old_style_widget.widget)

        text = "Apply"
        description = "Apply property changes"
        self.apply_button, tb = _create_button(
            text, description, icons.applyGrey, self._on_apply_clicked, layout)
        tb.setPopupMode(QToolButton.InstantPopup)

        text = "Decline"
        description = ("Decline property changes "
                       "and reset them to value on device")
        self.decline_button, _ = _create_button(
            text, description, icons.no, self._on_decline_clicked, layout)

        self.__busyTimer = QTimer(self)
        self.__busyTimer.setSingleShot(True)
        self.__busyTimer.timeout.connect(self._on_timeout)
        self.__current_value = None
        self.__has_conflict = False

        return layout

    def _on_apply_clicked(self):
        widget = self.old_style_widget
        network = []
        for b in widget.boxes:
            b.signalUserChanged.emit(b, widget.value, None)
            if b.configuration.type == "macro":
                b.set(widget.value)
            elif b.configuration.type == "deviceGroup":
                b.set(widget.value)
                # Broadcast changes for all devices belonging to this group
                for d in b.configuration.devices:
                    device_box = d.getBox(b.path)
                    device_box.set(widget.value)
                    # Send to network per device
                    Network().onReconfigure([(device_box, widget.value)])
            elif b.descriptor is not None:
                network.append((b, widget.value))

        if network:
            self.__busyTimer.start(5000)
            Network().onReconfigure(network)

    def _on_decline_clicked(self):
        widget = self.old_style_widget
        for b in widget.boxes:
            widget.valueChanged(b, self.__current_value)
        self._update_buttons()

    @pyqtSlot(object, object)
    def _on_display_value_change(self, box, value):
        widget = self.old_style_widget
        if self.__current_value is None:
            widget.valueChanged(box, value)
        self.__current_value = value
        self.__busyTimer.stop()
        self.__has_conflict = True
        self._update_buttons()

    def _on_editing_finished(self, box, value):
        if self.__current_value is None:
            return
        self._update_buttons()

    def _on_timeout(self):
        pass

    @pyqtSlot(object, object, object)
    def _on_user_edit(self, box, value, timestamp=None):
        self.old_style_widget.valueChangedSlot(box, value, timestamp)
        self._update_buttons()

    def _update_buttons(self):
        box = self.boxes[0]
        widget = self.old_style_widget
        allowed = box.isAllowed()
        self.apply_button.setEnabled(allowed)

        value_unchanged = determine_if_value_unchanged(
            self.__current_value, widget.value, box)
        if value_unchanged:
            self.apply_button.setIcon(icons.applyGrey)
            self.__has_conflict = False
            description = None
        elif self.__has_conflict:
            self.apply_button.setIcon(icons.applyConflict)
            description = "Apply my property changes"
        else:
            description = "Apply property changes"
            self.apply_button.setIcon(icons.apply)

        self.apply_button.setStatusTip(description)
        self.apply_button.setToolTip(description)
        self.decline_button.setEnabled(allowed and not value_unchanged)
