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
from qtpy.QtCore import QRect
from qtpy.QtWidgets import QWidget

from karabo.common.scenemodel.api import StickerModel
from karabogui.sceneview.widget.sticker import StickerDialog, StickerWidget
from karabogui.testing import GuiTestCase

TEXT = """
Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod
tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At
vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd
gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet."""


class TestStickerWidget(GuiTestCase):

    """Test for the Sticker Widget is limited as this relies heavily on
       CSS customizations. The only way to test these values is just by
       intercepting the return values from a mocked dialog"""

    # -----------------------------------------------------------------------
    # Test setup

    def setUp(self):
        super().setUp()
        self.model = model = StickerModel(text=TEXT, foreground='black',
                                          x=0, y=0, width=100, height=100)

        # The sticker widget needs to have a shown parent in order to
        # test the sizes.
        self.parent = QWidget()
        self.widget = StickerWidget(model=model, parent=self.parent)
        self.parent.show()

    def tearDown(self):
        super().tearDown()
        self.parent = None
        self.widget = None

    # -----------------------------------------------------------------------
    # Actual tests

    def test_basics(self):
        # Check widget properties
        assert self.widget.toPlainText() == self.model.text
        model_rect = QRect(self.model.x, self.model.y,
                           self.model.width, self.model.height)
        assert self.widget.rect() == model_rect

        # Check dialog properties. It is needed to be shown to process painting
        # and check sizes
        dialog = StickerDialog(model=self.model, parent=self.widget)
        dialog.show()
        dialog_sticker = dialog.leText

        assert dialog_sticker.size() == self.widget.size()
        assert dialog_sticker.toPlainText() == self.model.text

        are_viewports_equivalent = (dialog_sticker.viewport().size()
                                    == self.widget.viewport().size())
        assert are_viewports_equivalent
