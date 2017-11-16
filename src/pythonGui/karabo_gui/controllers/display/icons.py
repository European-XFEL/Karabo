from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QAction, QLabel
from traits.api import Instance, Type, on_trait_change

from karabo.common.scenemodel.api import (
    DigitIconsModel, SelectionIconsModel, TextIconsModel)
from karabo_gui import icons
from karabo_gui.binding.api import (
    BaseBinding, BaseBindingController, register_binding_controller,
    FloatBinding, Int8Binding, Int16Binding, Int32Binding, Int64Binding,
    Uint8Binding, Uint16Binding, Uint32Binding, Uint64Binding, StringBinding
)
from karabo_gui.controllers.icons_dialogs import (
    DigitDialog, SelectionDialog, TextDialog, IconItem
)
from karabo_gui.controllers.util import has_options

NUMERICAL_BINDINGS = (FloatBinding, Int8Binding, Int16Binding, Int32Binding,
                      Int64Binding, Uint8Binding, Uint16Binding, Uint32Binding,
                      Uint64Binding)


class _BaseIcons(BaseBindingController):
    def create_widget(self, parent):
        widget = QLabel(parent)
        action = QAction('Change Icons...', widget)
        widget.addAction(action)
        action.triggered.connect(self._on_change_icons)
        return widget

    @on_trait_change('model.values')
    def _force_update(self):
        """Handle updates to the `IconData` collection for all subclasses.
        We make sure to convert from plain IconData => IconItem here.
        """
        def _state(item):
            NAMES = ['equal', 'value', 'image', 'data']
            return {k: getattr(item, k) for k in NAMES}

        converted = [IconItem(**_state(it))
                     if not isinstance(it, IconItem) else it
                     for it in self.model.values]
        self.model.trait_setq(values=converted)

        # Maybe update the widget
        if self.widget is not None:
            self._value_update(self.proxy.value)

    @pyqtSlot()
    def _on_change_icons(self):
        binding = self.proxy.binding
        dialog = self.dialog_klass(self.model.values, binding)
        self.model.values = dialog.exec_()

    def set_pixmap(self, p):
        if p is None:
            self.widget.setPixmap(icons.no.pixmap(100))
        else:
            self.widget.setPixmap(p)


@register_binding_controller(ui_name='Icons', binding_type=NUMERICAL_BINDINGS,
                             is_compatible=has_options)
class DigitIcons(_BaseIcons):
    model = Instance(DigitIconsModel)
    dialog_klass = Type(DigitDialog)

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        for item in self.model.values:
            if (value < int(item.value) or
                    value == int(item.value) and item.equal):
                self.set_pixmap(item.pixmap)
                break


@register_binding_controller(ui_name='Selection Icons',
                             binding_type=BaseBinding.__subclasses__(),
                             is_compatible=has_options)
class SelectionIcons(_BaseIcons):
    model = Instance(SelectionIconsModel)
    dialog_klass = Type(SelectionDialog)

    @on_trait_change('proxy:binding')
    def _binding_update(self, binding):
        items = list(self.model.values)
        for opt in binding.options:
            if not any(opt == item.value for item in self.model.values):
                newItem = IconItem(value=opt)
                items.append(newItem)
        self.model.values = items

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        for item in self.model.values:
            if item.value == value:
                self.set_pixmap(item.pixmap)
                return

        msg = 'value "{}" of "{}" not in options ({})'
        raise RuntimeError(msg.format(value, self.proxy.key,
                                      self.proxy.binding.options))


@register_binding_controller(ui_name='Icons', binding_type=StringBinding,
                             is_compatible=has_options)
class TextIcons(_BaseIcons):
    model = Instance(TextIconsModel)
    dialog_klass = Type(TextDialog)

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        for it in self.model.values:
            if it.value is None or value is not None and it.re.match(value):
                self.set_pixmap(it.pixmap)
                return
