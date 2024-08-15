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
import numpy as np
from qtpy.QtGui import QImage

from karabogui.graph.image.api import karabo_default_image
from karabogui.graph.image.base import KaraboImageView
from karabogui.testing import GuiTestCase

# image dimensions
SMALL = (150, 100)
MEDIUM = (800, 600)
LARGE = (1600, 1200)
DIMENSIONS = [SMALL, MEDIUM, LARGE]
MIN_DOWNSAMPLING = [1, 1.5, 2]


class _BaseImageItemTest(GuiTestCase):
    def setUp(self):
        super().setUp()
        # Instantiate imageItem from KaraboImageView
        self.imageView = KaraboImageView()
        self.imageItem = self.imageView.plot().imageItem
        self.imageView.setFixedSize(300, 300)

    def tearDown(self):
        super().tearDown()
        self.imageView.destroy()
        self.imageView = None

    def set_image(self, image):
        # Set image data
        self.imageItem.setImage(image)
        self.process_qt_events()
        np.testing.assert_array_equal(image, self.imageItem.image)
        assert self.imageItem.qimage is None

        # Render QImage
        self.imageItem._viewBox().autoRange()
        self.imageItem._view_rect = self.imageItem.viewRect()
        self.imageItem.render()

    def assert_indexed_image(self, image, downsampling):
        self.set_image(image)
        self.assert_downsample(downsampling)
        self.assert_qimage(image, img_format=QImage.Format_Indexed8)

    def assert_rgb_image(self, image):
        self.set_image(image)
        self.assert_downsample(downsampling=None)
        self.assert_qimage(image, img_format=QImage.Format_ARGB32)

    def assert_downsample(self, downsampling):
        """Check downsample ratio and the resulting qimage dimensions"""
        if downsampling is None:
            assert self.imageItem._lastDownsample is None
        else:
            xds, yds = self.imageItem._lastDownsample
            assert xds >= downsampling
            assert yds >= downsampling

    def assert_qimage(self, image, *, img_format):
        qimage = self.imageItem.qimage
        height, width = image.shape[:2]
        if img_format == QImage.Format_Indexed8:
            # We expect that indexed format is downsampled
            xds, yds = self.imageItem._lastDownsample
            width = round(width / xds)
            height = round(height / yds)

        assert qimage.width() == width
        assert qimage.height() == height


class TestKaraboImageItem(_BaseImageItemTest):

    def test_rgb_images(self):
        for dims in DIMENSIONS:
            image = np.random.randint(0, 256, dims + (3,), dtype=np.uint8)
            self.assert_rgb_image(image)

    def test_rgba_images(self):
        for dims in DIMENSIONS:
            image = np.random.randint(0, 256, dims + (4,), dtype=np.uint8)
            self.assert_rgb_image(image)

    def test_alternating_image_types(self):
        """This checks when the image item receives alternating pseudocolor
           (indexed) and RGB images"""

        indexed_image = np.random.randint(0, 256, MEDIUM, dtype=np.uint8)
        rgb_image = np.random.randint(0, 256, MEDIUM + (4,), dtype=np.uint8)

        self.assert_indexed_image(indexed_image, downsampling=1.5)
        self.assert_rgb_image(rgb_image)
        self.assert_indexed_image(indexed_image, downsampling=1.5)

    def test_translation(self):
        """Test the translation of an image which is used by external clients
        """
        received = False

        def slotTransform():
            nonlocal received
            received = True

        dims = (800, 600)
        image = np.random.randint(0, 256, dims + (3,), dtype=np.uint8)
        self.assert_rgb_image(image)

        plotItem = self.imageView.plot()
        plotItem.imageTransformed.connect(slotTransform)

        np.testing.assert_array_equal(self.imageItem._origin,
                                      np.array([0, 0]))
        plotItem.set_translation(x_translate=10, y_translate=20)
        self.process_qt_events()

        np.testing.assert_array_equal(self.imageItem._origin,
                                      np.array([10, 20]))
        assert received
        self.assert_rgb_image(image)
        plotItem.imageTransformed.disconnect(slotTransform)


class Test2DImageItem(_BaseImageItemTest):

    def test_uint8_images(self):
        for dims, downsampling in zip(DIMENSIONS, MIN_DOWNSAMPLING):
            image = np.random.randint(0, 255, dims, dtype=np.uint8)
            self.assert_indexed_image(image, downsampling)

    def test_unint32_images(self):
        for dims, downsampling in zip(DIMENSIONS, MIN_DOWNSAMPLING):
            image = np.random.randint(0, 100000, dims, dtype=np.uint32)
            self.assert_indexed_image(image, downsampling)

    def test_int8_images(self):
        for dims, downsampling in zip(DIMENSIONS, MIN_DOWNSAMPLING):
            image = np.random.randint(-128, 128, dims, dtype=np.int8)
            self.assert_indexed_image(image, downsampling)

    def test_int32_images(self):
        for dims, downsampling in zip(DIMENSIONS, MIN_DOWNSAMPLING):
            image = np.random.randint(-50000, 50000, dims, dtype=np.int32)
            self.assert_indexed_image(image, downsampling)

    def test_float_images(self):
        for dims, downsampling in zip(DIMENSIONS, MIN_DOWNSAMPLING):
            image = np.random.uniform(-50000, 50000, dims)
            self.assert_indexed_image(image, downsampling)

    def test_set_graph_image(self):
        """Make sure setting levels with default image (all zero array)"""
        image = karabo_default_image()
        self.imageItem.setImage(image)
        self.imageItem._set_graph_image(autoLevels=True)
        assert np.array_equal(self.imageItem.levels, np.array([0, 1]))
