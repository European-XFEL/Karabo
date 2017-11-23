
from PyQt4.QtGui import QFrame, QHBoxLayout, QLabel, QSizePolicy


def add_unit_label(proxy, widget, parent=None):
    """Add a unit label to a widget, if available.

    NOTE: We are not deriving a class from QWidget here, because doing so
    breaks the usage of stylesheets elsewhere in the GUI (configurator).
    The `update_label` function is attached to the returned widget as a
    workaround...
    """
    unit_label = ''
    if proxy.binding is not None:
        unit_label = proxy.binding.unit_label

    widget_group = QFrame(parent)
    layout = QHBoxLayout(widget_group)
    layout.setSizeConstraint(QHBoxLayout.SetMinimumSize)
    layout.addWidget(widget)
    label = QLabel(unit_label, parent)
    label.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Preferred)
    layout.addWidget(label)
    layout.setContentsMargins(0, 0, 0, 0)
    # Add an `update_label` "method" for keeping things synced
    widget_group.update_label = lambda b: _updater(label, b)

    return widget_group


def _updater(label, proxy):
    """A clean way to update unit labels
    """
    if proxy.binding is None:
        return

    unit_label = proxy.binding.unit_label
    if unit_label != label.text():
        label.setText(unit_label)
