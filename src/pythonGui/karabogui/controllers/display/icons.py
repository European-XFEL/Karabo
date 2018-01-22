from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QAction, QLabel
from traits.api import Instance, Type, on_trait_change

from karabo.common.scenemodel.api import (
    DigitIconsModel, SelectionIconsModel, TextIconsModel)
from karabogui import icons
from karabogui.binding.api import (
    BaseBinding, FloatBinding, get_binding_value, IntBinding, StringBinding)
from karabogui.controllers.api import (
    BaseBindingController, has_options, register_binding_controller)
from karabogui.controllers.icons_dialogs import (
    DigitDialog, SelectionDialog, TextDialog, IconItem
)

NUMERICAL_BINDINGS = (FloatBinding, IntBinding)


class _BaseIcons(BaseBindingController):
    def create_widget(self, parent):
        widget = QLabel(parent)
        widget.setMinimumSize(24, 24)
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
        if (self.widget is not None and
                get_binding_value(self.proxy) is not None):
            self.value_update(self.proxy)

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
                             klassname='DigitIcons', is_compatible=has_options)
class DigitIcons(_BaseIcons):
    model = Instance(DigitIconsModel, args=())
    dialog_klass = Type(DigitDialog)

    def value_update(self, proxy):
        value = proxy.value
        for item in self.model.values:
            if (value < float(item.value) or
                    value == float(item.value) and item.equal):
                self.set_pixmap(item.pixmap)
                break


@register_binding_controller(ui_name='Selection Icons',
                             klassname='SelectionIcons',
                             binding_type=BaseBinding,
                             is_compatible=has_options)
class SelectionIcons(_BaseIcons):
    model = Instance(SelectionIconsModel, args=())
    dialog_klass = Type(SelectionDialog)

    def binding_update(self, proxy):
        items = list(self.model.values)
        for opt in proxy.binding.options:
            if not any(opt == item.value for item in self.model.values):
                newItem = IconItem(value=opt)
                items.append(newItem)
        self.model.values = items

    def value_update(self, proxy):
        value = proxy.value
        for item in self.model.values:
            if item.value == value:
                self.set_pixmap(item.pixmap)
                return

        msg = 'value "{}" of "{}" not in options ({})'
        raise RuntimeError(msg.format(value, self.proxy.key,
                                      self.proxy.binding.options))


@register_binding_controller(ui_name='Icons', binding_type=StringBinding,
                             klassname='TextIcons',
                             is_compatible=has_options)
class TextIcons(_BaseIcons):
    model = Instance(TextIconsModel, args=())
    dialog_klass = Type(TextDialog)

    def value_update(self, proxy):
        value = proxy.value
        for it in self.model.values:
            if it.re.match(value):
                self.set_pixmap(it.pixmap)
                return
