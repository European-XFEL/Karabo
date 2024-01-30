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
from pathlib import Path

from qtpy.QtCore import QSize
from qtpy.QtSvg import QSvgWidget
from qtpy.QtWidgets import QLabel, QWidget

from karabo.common.scenemodel.api import ImageRendererModel, create_base64image
from karabogui import icons
from karabogui.sceneview.widget.renderer import ImageRendererWidget
from karabogui.testing import GuiTestCase


class TestRenderers(GuiTestCase):

    def setUp(self):
        super().setUp()
        # The widget needs to have a shown parent in order to
        # test the sizes.
        self.parent = QWidget()
        self.parent.show()

    def tearDown(self):
        self.parent.destroy()
        self.parent = None

    def test_image_renderer(self):
        with self.subTest(msg="Test a valid svg file with renderer widget"):
            path = Path(icons.__file__).parent / "about.svg"
            with open(path, "rb") as fp:
                b = fp.read()

            image = create_base64image("svg", b)
            model = ImageRendererModel(
                x=0, y=0, image=image, width=100, height=100)
            widget = ImageRendererWidget(model=model, parent=self.parent)

            assert widget.size() == QSize(100, 100)
            assert isinstance(widget._internal_widget, QSvgWidget)
            # Valid renderer
            assert widget._internal_widget.renderer().isValid()
            widget.destroy()

        with self.subTest(msg="Test png file with renderer widget"):
            path = Path(icons.__file__).parent / "splash.png"
            with open(path, "rb") as fp:
                b = fp.read()

            image = create_base64image("png", b)
            model = ImageRendererModel(
                x=0, y=0, image=image, width=50, height=50)

            widget = ImageRendererWidget(model=model, parent=self.parent)
            assert isinstance(widget._internal_widget, QLabel)
            assert widget.size() == QSize(50, 50)
            widget.destroy()
