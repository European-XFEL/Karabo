# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import re

from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtGui import QPalette, QValidator
from qtpy.QtWidgets import QDialog, QDialogButtonBox

from .utils import get_dialog_ui

ul = '\u00a1-\uffff'  # unicode letters range (must not be a raw string)

# IP patterns
ipv4_re = r'(?:25[0-5]|2[0-4]\d|[0-1]?\d?\d)(?:\.(?:25[0-5]|2[0-4]\d|[0-1]?\d?\d)){3}'  # noqa
ipv6_re = r'\[[0-9a-f:\.]+\]'  # (simple regex, validated later)

# Host patterns
hostname_re = r'[a-z' + ul + r'0-9](?:[a-z' + ul + r'0-9-]{0,61}[a-z' + ul + r'0-9])?'  # noqa
# Max length for domain name labels is 63 characters per RFC 1034 sec. 3.1
domain_re = r'(?:\.(?!-)[a-z' + ul + r'0-9-]{1,63}(?<!-))*'
tld_re = (
    r'\.'  # dot
    r'(?!-)'  # can't start with a dash
    r'(?:[a-z' + ul + '-]{2,63}'  # domain label
    r'|xn--[a-z0-9]{1,59})'  # or punycode label
    r'(?<!-)'  # can't end with a dash
    r'\.?'  # may have a trailing dot
)
host_re = '(' + hostname_re + domain_re + tld_re + '|localhost)'

karabo_re = '(gui|cinema|theatre)'

URL_REGEX = re.compile(
    r'^(?:[a-z0-9\.\-\+]*)://'  # scheme is validated separately
    r'(?:[^\s:@/]+(?::[^\s:@/]*)?@)?'  # user:pass authentication
    r'(?:' + ipv4_re + '|' + ipv6_re + '|' + host_re + '|' + karabo_re + ')'
    r'(?::\d{2,5})?'  # port
    r'(?:[/?#][^\s]*)?'  # resource path
    r'\Z', re.IGNORECASE)

SCHEME_URL_REGEX = re.compile(r'^([a-zA-Z]+)://')

SCHEMES = ['http', 'https', 'ftp', 'ftps', 'karabo']


def is_valid_url(url):
    """Is the link string a valid URL

    -> https://github.com/django/django/blob/master/django/core/validators.py
    """
    if not url:
        return False

    # Add a schema if None
    scheme_match = SCHEME_URL_REGEX.match(url)
    if not scheme_match:
        url = SCHEMES[0] + "://" + url
    else:
        scheme = scheme_match.group(1).lower()
        if scheme not in SCHEMES:
            return False

    return URL_REGEX.match(url)


class WebValidator(QValidator):
    def __init__(self, parent=None):
        super().__init__(parent)

    def validate(self, input, pos):
        if not is_valid_url(input):
            return self.Intermediate, input, pos
        return self.Acceptable, input, pos


class WebDialog(QDialog):
    """A dialog to modify the layout bounding rect coordinates
    """

    def __init__(self, target='http://', title='Weblink', parent=None):
        super().__init__(parent)
        filepath = get_dialog_ui('webdialog.ui')
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
        self._on_validate()

    @Slot()
    def _on_validate(self):
        acceptable_input = self.ui_target.hasAcceptableInput()
        palette = (self._normal_palette if acceptable_input
                   else self._error_palette)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(
            acceptable_input)
        self.ui_target.setPalette(palette)

    @property
    def target(self):
        target = self.ui_target.text()
        if not SCHEME_URL_REGEX.match(target):
            target = SCHEMES[0] + "://" + target

        return target
