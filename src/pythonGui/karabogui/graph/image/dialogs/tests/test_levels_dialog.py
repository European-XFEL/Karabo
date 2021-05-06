import numpy as np

from karabogui.graph.image.plot import KaraboImagePlot
from karabogui.testing import GuiTestCase

from ..levels import LevelsDialog


class TestLevelsDialog(GuiTestCase):

    def setUp(self):
        super(TestLevelsDialog, self).setUp()
        self.plotItem = KaraboImagePlot()
        self.imageItem = self.plotItem.imageItem

    def tearDown(self):
        self.imageItem.deleteLater()
        self.imageItem = None

    def test_image_autolevel(self):
        """Tests the dialog initialization for imageItems with autolevels"""

        # Setup levels dialog
        image = np.random.randint(-100, 100, (100, 100))
        image_range = image.min(), image.max()

        self.imageItem.setImage(image)
        image_levels = self.imageItem.levels
        auto_levels = self.imageItem.auto_levels

        # Do some prior checking before the actual tests
        np.testing.assert_array_equal(image_levels, image_range)
        self.assertTrue(auto_levels)

        # Instantiate dialog
        dialog = LevelsDialog(image_levels, image_range, auto_levels)

        # Check dialog widgets if enabled
        self.assertTrue(dialog.automatic_checkbox.isChecked())
        self.assertFalse(dialog.min_spinbox.isEnabled())
        self.assertFalse(dialog.max_spinbox.isEnabled())

        # Check dialog widget values
        self.assertEqual(dialog.min_spinbox.value(), image_levels[0])
        self.assertEqual(dialog.max_spinbox.value(), image_levels[1])

    def test_image_not_autolevel(self):
        """Tests the dialog initialization for imageItems with
           enforced levels"""

        # Setup levels dialog
        image = np.random.randint(-100, 100, (100, 100))
        input_levels = [-50, 50]

        self.imageItem.setImage(image)
        self.plotItem.set_image_levels(input_levels)
        image_levels = self.imageItem.levels
        auto_levels = self.imageItem.auto_levels

        # Do some prior checking before the actual tests
        np.testing.assert_array_equal(image_levels, input_levels)
        self.assertFalse(auto_levels)

        # Instantiate dialog
        dialog = LevelsDialog(image_levels, input_levels, auto_levels)

        # Check dialog widgets if enabled
        self.assertFalse(dialog.automatic_checkbox.isChecked())
        self.assertTrue(dialog.min_spinbox.isEnabled())
        self.assertTrue(dialog.max_spinbox.isEnabled())

        # Check dialog widget values
        self.assertEqual(dialog.min_spinbox.value(), image_levels[0])
        self.assertEqual(dialog.min_label.text(), str(image_levels[0]))
        self.assertEqual(dialog.max_spinbox.value(), image_levels[1])
        self.assertEqual(dialog.max_label.text(), str(image_levels[1]))

    def test_accepted_autolevel(self):
        """Check if accepted values are input values"""

        # Setup levels dialog
        image = np.random.randint(-100, 100, (100, 100))
        image_range = image.min(), image.max()

        input_levels = [-50, 50]
        self.imageItem.setImage(image)
        self.plotItem.set_image_levels(input_levels)

        # Instantiate dialog
        dialog = LevelsDialog(self.imageItem.levels, image_range,
                              self.imageItem.auto_levels)

        # Change the values then accept the dialog
        dialog.automatic_checkbox.setChecked(True)
        dialog.accept()

        # Check if changes are saved
        self.assertIsNone(dialog.levels)

    def test_accepted_not_autolevel(self):
        # Setup levels dialog
        image = np.random.randint(-100, 100, (100, 100))
        image_range = image.min(), image.max()

        self.imageItem.setImage(image)

        # Instantiate dialog
        dialog = LevelsDialog(self.imageItem.levels, image_range,
                              self.imageItem.auto_levels)

        # Change the values then accept the dialog
        input_levels = [-50, 50]
        dialog.automatic_checkbox.setChecked(False)
        dialog.min_spinbox.setValue(input_levels[0])
        dialog.max_spinbox.setValue(input_levels[1])
        dialog.accept()

        # Check if changes are saved
        np.testing.assert_array_equal(dialog.levels, input_levels)
