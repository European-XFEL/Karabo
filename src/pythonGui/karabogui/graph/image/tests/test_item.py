import unittest

import numpy as np

from karabogui.graph.image.base import KaraboImageView
from karabogui.testing import GuiTestCase


class TestKaraboImageItem(GuiTestCase):

    def setUp(self):
        super().setUp()
        # Instantiate imageItem from KaraboImageView
        self.imageView = KaraboImageView()
        self.imageItem = self.imageView.plot().imageItem
        self.imageItem.enable_downsampling(True)
        self.imageView.setFixedSize(300, 300)

    def tearDown(self):
        super(TestKaraboImageItem, self).tearDown()
        self.imageView.destroy()
        self.imageView = None
        self.imageItem = None

    def test_uint8_2D_image_small(self):
        """Tests uint8 image with dimensions of 150 x 100"""

        HEIGHT, WIDTH = (150, 100)

        # Setup imageItem with test image
        image = np.random.randint(0, 256, (HEIGHT, WIDTH), dtype=np.uint8)
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()
        self.assertEqual(self.imageItem._lastDownsample, (1, 1))
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertEqual(qimage.width(), WIDTH)
        self.assertEqual(qimage.height(), HEIGHT)

    def test_uint8_2D_image_medium(self):
        """Tests uint8 image with dimensions of 800 x 600"""

        HEIGHT, WIDTH = (800, 600)

        # Setup imageItem with test image
        image = np.random.randint(0, 256, (HEIGHT, WIDTH), dtype=np.uint8)
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()

        # Check downsample ratio
        xds, yds = self.imageItem._lastDownsample
        self.assertGreaterEqual(xds, 1.5)
        self.assertGreaterEqual(yds, 1.5)

        # Check generated QImage
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertLess(qimage.width(), WIDTH)
        self.assertLess(qimage.height(), HEIGHT)

    def test_uint8_2D_image_large(self):
        """Tests uint8 image with dimensions of 1600 x 1200"""

        HEIGHT, WIDTH = (1600, 1200)

        # Setup imageItem with test image
        image = np.random.randint(0, 256, (HEIGHT, WIDTH), dtype=np.uint8)
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()

        # Check downsample ratio
        xds, yds = self.imageItem._lastDownsample
        self.assertGreaterEqual(xds, 2)
        self.assertGreaterEqual(yds, 2)

        # Check generated QImage
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertLess(qimage.width(), WIDTH)
        self.assertLess(qimage.height(), HEIGHT)

    def test_uint32_2D_image_small(self):
        """Tests uint8 image with dimensions of 150 x 100"""

        HEIGHT, WIDTH = (150, 100)

        # Setup imageItem with test image
        image = np.random.randint(0, 100000, (HEIGHT, WIDTH), dtype=np.uint32)
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()
        self.assertEqual(self.imageItem._lastDownsample, (1, 1))
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertEqual(qimage.width(), WIDTH)
        self.assertEqual(qimage.height(), HEIGHT)

    def test_uint32_2D_image_medium(self):
        """Tests uint8 image with dimensions of 800 x 600"""

        HEIGHT, WIDTH = (800, 600)

        # Setup imageItem with test image
        image = np.random.randint(0, 100000, (HEIGHT, WIDTH), dtype=np.uint32)
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()

        # Check downsample ratio
        xds, yds = self.imageItem._lastDownsample
        self.assertGreaterEqual(xds, 1.5)
        self.assertGreaterEqual(yds, 1.5)

        # Check generated QImage
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertLess(qimage.width(), WIDTH)
        self.assertLess(qimage.height(), HEIGHT)

    def test_uint32_2D_image_large(self):
        """Tests uint8 image with dimensions of 1600 x 1200"""

        HEIGHT, WIDTH = (1600, 1200)

        # Setup imageItem with test image
        image = np.random.randint(0, 100000, (HEIGHT, WIDTH), dtype=np.uint32)
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()

        # Check downsample ratio
        xds, yds = self.imageItem._lastDownsample
        self.assertGreaterEqual(xds, 2)
        self.assertGreaterEqual(yds, 2)

        # Check generated QImage
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertLess(qimage.width(), WIDTH)
        self.assertLess(qimage.height(), HEIGHT)

    def test_int8_2D_image_small(self):
        """Tests uint8 image with dimensions of 150 x 100"""

        HEIGHT, WIDTH = (150, 100)

        # Setup imageItem with test image
        image = np.random.randint(-128, 128, (HEIGHT, WIDTH), dtype=np.int8)
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()
        self.assertEqual(self.imageItem._lastDownsample, (1, 1))
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertEqual(qimage.width(), WIDTH)
        self.assertEqual(qimage.height(), HEIGHT)

    def test_int8_2D_image_medium(self):
        """Tests uint8 image with dimensions of 800 x 600"""

        HEIGHT, WIDTH = (800, 600)

        # Setup imageItem with test image
        image = np.random.randint(-128, 128, (HEIGHT, WIDTH), dtype=np.int8)
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()

        # Check downsample ratio
        xds, yds = self.imageItem._lastDownsample
        self.assertGreaterEqual(xds, 1.5)
        self.assertGreaterEqual(yds, 1.5)

        # Check generated QImage
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertLess(qimage.width(), WIDTH)
        self.assertLess(qimage.height(), HEIGHT)

    def test_int8_2D_image_large(self):
        """Tests uint8 image with dimensions of 1600 x 1200"""

        HEIGHT, WIDTH = (1600, 1200)

        # Setup imageItem with test image
        image = np.random.randint(-128, 128, (HEIGHT, WIDTH), dtype=np.int8)
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()

        # Check downsample ratio
        xds, yds = self.imageItem._lastDownsample
        self.assertGreaterEqual(xds, 2)
        self.assertGreaterEqual(yds, 2)

        # Check generated QImage
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertLess(qimage.width(), WIDTH)
        self.assertLess(qimage.height(), HEIGHT)

    def test_int32_2D_image_small(self):
        """Tests uint8 image with dimensions of 150 x 100"""

        HEIGHT, WIDTH = (150, 100)

        # Setup imageItem with test image
        image = np.random.randint(-50000, 50000, (HEIGHT, WIDTH),
                                  dtype=np.int32)
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()
        self.assertEqual(self.imageItem._lastDownsample, (1, 1))
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertEqual(qimage.width(), WIDTH)
        self.assertEqual(qimage.height(), HEIGHT)

    def test_int32_2D_image_medium(self):
        """Tests uint8 image with dimensions of 800 x 600"""

        HEIGHT, WIDTH = (800, 600)

        # Setup imageItem with test image
        image = np.random.randint(-50000, 50000, (HEIGHT, WIDTH),
                                  dtype=np.int32)
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()

        # Check downsample ratio
        xds, yds = self.imageItem._lastDownsample
        self.assertGreaterEqual(xds, 1.5)
        self.assertGreaterEqual(yds, 1.5)

        # Check generated QImage
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertLess(qimage.width(), WIDTH)
        self.assertLess(qimage.height(), HEIGHT)

    def test_int32_2D_image_large(self):
        """Tests uint8 image with dimensions of 1600 x 1200"""

        HEIGHT, WIDTH = (1600, 1200)

        # Setup imageItem with test image
        image = np.random.randint(-50000, 50000, (HEIGHT, WIDTH),
                                  dtype=np.int32)
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()

        # Check downsample ratio
        xds, yds = self.imageItem._lastDownsample
        self.assertGreaterEqual(xds, 2)
        self.assertGreaterEqual(yds, 2)

        # Check generated QImage
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertLess(qimage.width(), WIDTH)
        self.assertLess(qimage.height(), HEIGHT)

    def test_pos_float_2D_image_small(self):
        """Tests uint8 image with dimensions of 150 x 100"""

        HEIGHT, WIDTH = (150, 100)

        # Setup imageItem with test image
        image = np.random.uniform(-50000, 50000, (HEIGHT, WIDTH))
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()
        self.assertEqual(self.imageItem._lastDownsample, (1, 1))
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertEqual(qimage.width(), WIDTH)
        self.assertEqual(qimage.height(), HEIGHT)

    def test_pos_float_2D_image_medium(self):
        """Tests uint8 image with dimensions of 800 x 600"""

        HEIGHT, WIDTH = (800, 600)

        # Setup imageItem with test image
        image = np.random.uniform(-50000, 50000, (HEIGHT, WIDTH))
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()

        # Check downsample ratio
        xds, yds = self.imageItem._lastDownsample
        self.assertGreaterEqual(xds, 1.5)
        self.assertGreaterEqual(yds, 1.5)

        # Check generated QImage
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertLess(qimage.width(), WIDTH)
        self.assertLess(qimage.height(), HEIGHT)

    def test_pos_float_2D_image_large(self):
        """Tests uint8 image with dimensions of 1600 x 1200"""

        HEIGHT, WIDTH = (1600, 1200)

        # Setup imageItem with test image
        image = np.random.uniform(-50000, 50000, (HEIGHT, WIDTH))
        self.imageItem.setImage(image)
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem.render()

        # Check downsample ratio
        xds, yds = self.imageItem._lastDownsample
        self.assertGreaterEqual(xds, 2)
        self.assertGreaterEqual(yds, 2)

        # Check generated QImage
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertLess(qimage.width(), WIDTH)
        self.assertLess(qimage.height(), HEIGHT)