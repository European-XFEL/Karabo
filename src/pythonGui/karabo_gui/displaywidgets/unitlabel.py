
from PyQt4.QtGui import QFrame, QHBoxLayout, QLabel, QSizePolicy


def add_unit_label(box, widget, parent=None):
    """Add a unit label to a widget, if available.

    NOTE: We are not deriving a class from QWidget here, because doing so
    breaks the usage of stylesheets elsewhere in the GUI (configurator).
    The `updateLabel` function is attached to the returned widget as a
    workaround...
    """
    unit_label = ''
    if box is not None:
        unit_label = box.unitLabel()

    widget_group = QFrame(parent)
    layout = QHBoxLayout(widget_group)
    layout.setSizeConstraint(QHBoxLayout.SetMinimumSize)
    layout.addWidget(widget)
    label = QLabel(unit_label, parent)
    label.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Preferred)
    layout.addWidget(label)
    layout.setContentsMargins(0, 0, 0, 0)
    # Add an `updateLabel` "method" for keeping things synced
    widget_group.updateLabel = lambda b: _updater(label, b)

    return widget_group


def _updater(label, box):
    """A clean way to update unit labels
    """
    if box is None:
        return

    unit_label = box.unitLabel()
    if unit_label != label.text():
        label.setText(unit_label)
