from qtpy import uic
from qtpy.QtCore import QObject, Qt, Slot
from qtpy.QtGui import QPalette
from qtpy.QtWidgets import QDialog
from traits.api import Undefined

from karabo.native import AccessMode, Assignment, Hash, create_html_hash
from karabogui.binding.api import DeviceClassProxy, iterate_binding
from karabogui.dialogs.utils import get_dialog_ui
from karabogui.singletons.api import get_topology
from karabogui.util import get_spin_widget

CONFIGURATION_PAGE = 0
WAITING_PAGE = 1
INIT_CONFIG = "Init Configuration"
CONFIG_WITH_DEFAULTS = "Init Configuration with Schema defaults"


def extract_defaults(binding) -> Hash:
    """Extract the default values from the class proxy binding."""
    ret = Hash()
    for key, node in iterate_binding(binding):
        read_only = node.accessMode is AccessMode.READONLY
        is_internal = node.assignment is Assignment.INTERNAL
        if read_only or is_internal:
            continue
        default = node.value
        if default is Undefined:
            continue
        ret[key] = default

    return ret


class InitConfigurationPreviewDialog(QDialog):
    def __init__(self, proxy: DeviceClassProxy, configuration: Hash,
                 title: str, parent: QObject | None = None):

        super().__init__(parent)

        ui_file = get_dialog_ui("init_configuration_preview.ui")
        uic.loadUi(ui_file, self)
        self.setWindowTitle(title)
        self.configuration = configuration
        self._config_with_defaults = None

        wait_widget = get_spin_widget(icon='wait', parent=self)
        wait_widget.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        wait_widget.setAutoFillBackground(True)
        wait_widget.setBackgroundRole(QPalette.Base)
        self.stackedWidget.addWidget(wait_widget)

        self.tabWidget.setTabText(0, INIT_CONFIG)
        self.tabWidget.setTabText(1, CONFIG_WITH_DEFAULTS)

        self.ui_configuration_selector.addItems(
            [INIT_CONFIG, CONFIG_WITH_DEFAULTS])
        self.ui_configuration_selector.currentIndexChanged.connect(
            self._update_info_label)

        server_id = proxy.server_id
        class_id = proxy.binding.class_id

        class_proxy = get_topology().get_class(server_id, class_id)
        if not len(class_proxy.binding.value) > 0:

            def config_handler():
                class_proxy.on_trait_change(
                    config_handler, "config_update", remove=True)
                self.show_configuration(class_proxy.binding)

            class_proxy.on_trait_change(config_handler, "config_update")

            # Schema has not arrived yet. Let's show the waiting page.
            self.stackedWidget.setCurrentIndex(WAITING_PAGE)
        else:
            self.show_configuration(class_proxy.binding)

        self._update_info_label(index=0)

    @property
    def configuration_to_apply(self) -> Hash:
        if self.ui_configuration_selector.currentText() == INIT_CONFIG:
            return self.configuration
        return self._config_with_defaults

    def show_configuration(self, binding):
        default_values = extract_defaults(binding)
        default_values.merge(self.configuration)
        self._config_with_defaults = default_values
        self.ui_configuration_text.setHtml(create_html_hash(
            self.configuration))
        self.ui_full_configuration_text.setHtml(create_html_hash(
            default_values))
        self.stackedWidget.setCurrentIndex(CONFIGURATION_PAGE)

    @Slot(int)
    def _update_info_label(self, index):
        if index == 0:
            text = ("Only the init configuration is applied to the "
                    "Configuration Editor.")
        else:
            text = ("The init configuration with class schema defaults "
                    "is applied to the Configuration Editor.")
        self.ui_info_label.setText(text)
