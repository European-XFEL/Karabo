from pathlib import Path

from qtpy.QtCore import QSize
from qtpy.QtSvg import QSvgWidget
from qtpy.QtWidgets import QLabel, QWidget

from karabo.common.scenemodel.api import (
    ImageRendererModel, convert_to_svg_image)
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

            image = convert_to_svg_image("svg", b)
            model = ImageRendererModel(
                x=0, y=0, image=image, width=100, height=100)
            widget = ImageRendererWidget(model=model, parent=self.parent)

            self.assertEqual(widget.size(), QSize(100, 100))
            self.assertIsInstance(widget._internal_widget, QSvgWidget)
            # Valid renderer
            self.assertTrue(widget._internal_widget.renderer().isValid())
            widget.destroy()

        with self.subTest(msg="Test png file with renderer widget"):
            path = Path(icons.__file__).parent / "splash.png"
            with open(path, "rb") as fp:
                b = fp.read()

            image = convert_to_svg_image("png", b)
            model = ImageRendererModel(
                x=0, y=0, image=image, width=50, height=50)

            widget = ImageRendererWidget(model=model, parent=self.parent)
            self.assertIsInstance(widget._internal_widget, QLabel)
            self.assertEqual(widget.size(), QSize(50, 50))
            widget.destroy()