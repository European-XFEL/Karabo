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
import random
from io import StringIO
from pathlib import Path

from lxml import etree
from qtpy.QtCore import QSize, Qt, Slot
from qtpy.QtGui import QPixmap
from qtpy.QtWidgets import (
    QCheckBox, QLabel, QPushButton, QVBoxLayout, QWizard, QWizardPage)

from karabogui import icons
from karabogui.singletons.api import get_config


def _get_notes():
    """Get the `Tips & Tricks` Notes and return a dictionary with
    `title` and `text` in a random fashion.
    """
    folder = Path(__file__).parent

    ret = {}

    def get_lines(fn):
        with open(fn) as fp:
            xml = fp.read()
            tree = etree.parse(StringIO(xml))
            title = tree.find('.//title').text
            ret.update({title: xml})

    files = [filename for filename in folder.rglob("*.html")]
    shuffle = random.sample(files, len(files))
    for fn in shuffle:
        get_lines(fn)

    return ret


LOGO_PATH = str(Path(icons.__file__).parent / Path('splash.png'))
LOGO_WIDTH = 100


class TipsTricksWizard(QWizard):

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setWindowTitle("Karabo Tips & Tricks")
        self.setWizardStyle(QWizard.ClassicStyle)
        checkbox = QCheckBox("Don't show tips at startup!")
        show_wizard = get_config()['wizard']
        checkbox.setChecked(not show_wizard)
        checkbox.stateChanged.connect(self.update_start)
        self.setButton(QWizard.CustomButton1, checkbox)
        self.setOption(QWizard.HaveCustomButton1)

        close_button = QPushButton("OK")
        close_button.clicked.connect(self.close)
        self.setButton(QWizard.CustomButton2, close_button)
        self.setOption(QWizard.HaveCustomButton2)

        layout = [QWizard.CustomButton1,
                  QWizard.BackButton,
                  QWizard.NextButton,
                  QWizard.CustomButton2]
        self.setButtonLayout(layout)

        self.button(QWizard.BackButton).setObjectName("BackButton")
        self.button(QWizard.NextButton).setObjectName("NextButton")

        self._build_information_pages()

    def sizeHint(self):
        return QSize(400, 200)

    def _build_information_pages(self):
        """Build basic information pages for the wizard"""
        pixmap = QPixmap(LOGO_PATH)
        pixmap = pixmap.scaledToWidth(LOGO_WIDTH)
        for title, text in _get_notes().items():
            page = QWizardPage(parent=self)
            page.setTitle(title)
            page.setPixmap(QWizard.WatermarkPixmap, pixmap)
            label = QLabel(text)
            label.setWordWrap(True)
            layout = QVBoxLayout()
            layout.addWidget(label)
            page.setLayout(layout)

            self.addPage(page)

    @Slot(int)
    def update_start(self, state):
        show_wizard = not bool(state)
        get_config()['wizard'] = show_wizard
