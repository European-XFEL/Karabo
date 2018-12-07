import os.path as op
import re

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot, QPoint, Qt
from PyQt4.QtGui import QDialog, QDialogButtonBox, QValidator, QPalette

URL_REGEX = re.compile(
    r'^(?:http|ftp)s?://'  # http:// or https://
    r'(?:(?:[A-Z0-9](?:[A-Z0-9-]{0,61}[A-Z0-9])?\.)+(?:[A-Z]{2,6}\.?|[A-Z0-9-]{2,}\.?)|'  # noqa  # domain...
    r'localhost|'  # localhost...
    r'\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})'  # ...or ip
    r'(?::\d+)?'  # optional port
    r'(?:/?|[/?]\S+)$', re.IGNORECASE)


def is_valid_url(url):
    """Is the link string a valid URL

    -> github.com/django/django/blob/stable/1.3.x/django/core/validators.py#L45
    """
    return url is not None and URL_REGEX.search(url)


class WebValidator(QValidator):
    def __init__(self, parent=None):
        super(WebValidator, self).__init__(parent)

    def validate(self, input, pos):
        if not is_valid_url(input):
            return self.Intermediate, input, pos
        return self.Acceptable, input, pos


class WebDialog(QDialog):
    """A dialog to modify the layout bounding rect coordinates
    """

    def __init__(self, target='http://', title='Weblink', parent=None):
        super(WebDialog, self).__init__(parent)
        filepath = op.join(op.abspath(op.dirname(__file__)),
                           'webdialog.ui')
        uic.loadUi(filepath, self)
        self.setModal(False)
        self._normal_palette = self.ui_target.palette()
        self._error_palette = QPalette(self._normal_palette)
        self._error_palette.setColor(QPalette.Text, Qt.red)

        validator = WebValidator()
        self.ui_target.setValidator(validator)
        self.ui_target.setText(target)
        self.ui_target.textChanged.connect(self._on_validate)
        # Fill the dialog with start values!
        self.setWindowTitle(title)
        if parent is not None:
            # place dialog accordingly!
            point = parent.rect().bottomRight()
            global_point = parent.mapToGlobal(point)
            self.move(global_point - QPoint(self.width(), 0))
        self._on_validate()

    @pyqtSlot()
    def _on_validate(self):
        acceptable_input = self.ui_target.hasAcceptableInput()
        palette = (self._normal_palette if acceptable_input
                   else self._error_palette)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(
            acceptable_input)
        self.ui_target.setPalette(palette)

    @property
    def target(self):
        return self.ui_target.text()
