import numpy as np

from karabogui.graph.image.base import KaraboImageView
from karabogui.testing import GuiTestCase

# image dimensions
SMALL = (150, 100)
MEDIUM = (800, 600)
LARGE = (1600, 1200)
DIMENSIONS = [SMALL, MEDIUM, LARGE]
MIN_DOWNSAMPLING = [1, 1.5, 2]


class TestKaraboImageItem(GuiTestCase):

    def setUp(self):
        super().setUp()
        # Instantiate imageItem from KaraboImageView
        self.imageView = KaraboImageView()
        self.imageItem = self.imageView.plot().imageItem
        self.imageView.setFixedSize(300, 300)

    def tearDown(self):
        super(TestKaraboImageItem, self).tearDown()
        self.imageView.destroy()
        self.imageView = None

    def test_uint8_images(self):
        for dims, downsampling in zip(DIMENSIONS, MIN_DOWNSAMPLING):
            image = np.random.randint(0, 256, dims, dtype=np.uint8)
            self._assert_render(image, downsampling)

    def test_unint32_images(self):
        for dims, downsampling in zip(DIMENSIONS, MIN_DOWNSAMPLING):
            image = np.random.randint(0, 100000, dims, dtype=np.uint32)
            self._assert_render(image, downsampling)

    def test_int8_images(self):
        for dims, downsampling in zip(DIMENSIONS, MIN_DOWNSAMPLING):
            image = np.random.randint(-128, 128, dims, dtype=np.int8)
            self._assert_render(image, downsampling)

    def test_int32_images(self):
        for dims, downsampling in zip(DIMENSIONS, MIN_DOWNSAMPLING):
            image = np.random.randint(-50000, 50000, dims, dtype=np.int32)
            self._assert_render(image, downsampling)

    def test_float_images(self):
        for dims, downsampling in zip(DIMENSIONS, MIN_DOWNSAMPLING):
            image = np.random.uniform(-50000, 50000, dims)
            self._assert_render(image, downsampling)

    def _assert_render(self, image, downsampling):
        self.imageItem.setImage(image)
        self.process_qt_events()
        np.testing.assert_array_equal(image, self.imageItem.image)
        self.assertIsNone(self.imageItem.qimage)

        # Render QImage
        self.imageItem._viewBox().autoRange()
        self.imageItem._view_rect = self.imageItem.viewRect()
        self.imageItem.render()

        # self.assertEqual(self.imageItem._lastDownsample, (1, 1))
        # Check downsample ratio
        xds, yds = self.imageItem._lastDownsample
        self.assertGreaterEqual(xds, downsampling)
        self.assertGreaterEqual(yds, downsampling)
        qimage = self.imageItem.qimage
        self.assertIsNotNone(qimage)
        self.assertEqual(qimage.width(), round(image.shape[1] / xds))
        self.assertEqual(qimage.height(), round(image.shape[0] / yds))
