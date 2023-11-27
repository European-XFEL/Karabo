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
from numpy.testing import assert_allclose
from qtpy.QtCore import Slot

from karabogui.graph.image.dialogs.levels import LevelsDialog, get_decimals
from karabogui.graph.image.plot import KaraboImagePlot
from karabogui.testing import GuiTestCase


class TestLevelsDialog(GuiTestCase):

    def setUp(self):
        super().setUp()
        self.plotItem = KaraboImagePlot()
        self.imageItem = self.plotItem.imageItem

    def tearDown(self):
        super().tearDown()
        self.imageItem.deleteLater()
        self.imageItem = None

    def test_image_preview_signal(self):
        image = np.random.randint(-100, 100, (100, 100))
        image_range = image.min(), image.max()

        self.imageItem.setImage(image)
        image_levels = self.imageItem.levels
        auto_levels = self.imageItem.auto_levels
        self.assertTrue(auto_levels)
        dialog = LevelsDialog(image_levels, image_range, auto_levels)

        # ----------------------------------------------------------------
        # Change auto level and cross check with a slot

        self.assertTrue(dialog.automatic_checkbox.isChecked())
        self.assertFalse(dialog.min_spinbox.isEnabled())
        self.assertFalse(dialog.max_spinbox.isEnabled())

        received = None
        count = 0

        @Slot()
        def levelUpdate(levels):
            nonlocal received, count
            received = levels
            count += 1

        dialog.levelsPreview.connect(levelUpdate)

        # Now change the auto levels
        self.assertIsNone(received, None)
        dialog.automatic_checkbox.setChecked(False)
        self.assertIsNotNone(dialog.levels, None)
        self.assertEqual(count, 1)
        self.assertIsNotNone(received)
        dialog.automatic_checkbox.setChecked(True)
        self.assertEqual(dialog.levels, None)
        self.assertEqual(count, 2)
        self.assertIsNone(received)

        # ----------------------------------------------------------------
        # Change spin boxes

        dialog.automatic_checkbox.setChecked(False)
        self.assertTrue(dialog.min_spinbox.isEnabled())
        self.assertTrue(dialog.max_spinbox.isEnabled())
        count = 0

        dialog.min_spinbox.setValue(2)
        self.assertEqual(count, 1)
        self.assertEqual(received, [2.0, 99.0])
        dialog.max_spinbox.setValue(55)
        self.assertEqual(count, 2)
        self.assertEqual(received, [2.0, 55.0])

        # For now, `slider.setValue` does not emit
        dialog.slider.setValue(13, 44)
        self.assertEqual(count, 2)

        # Finally disconnect
        dialog.levelsPreview.disconnect(levelUpdate)

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

        # Check slider range
        self.assertFalse(dialog.slider.isEnabled())
        self.assertEqual(dialog.slider.minimum(), image_range[0])
        self.assertEqual(dialog.slider.maximum(), image_range[1])

    def test_image_float_range(self):
        # Setup levels dialog
        image = np.random.randn(100, 100)
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
        assert_allclose(dialog.min_spinbox.value(), image_levels[0], atol=0.1)
        assert_allclose(dialog.max_spinbox.value(), image_levels[1], atol=0.1)

        # Check slider range
        self.assertFalse(dialog.slider.isEnabled())

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

    def test_accepted_not_autolevel_false_input(self):
        # Setup levels dialog
        image = np.random.randint(-100, 100, (100, 100))
        image_range = image.min(), image.max()

        self.imageItem.setImage(image)
        min_limit = -70
        max_limit = 200

        preview_levels = None

        @Slot(object)
        def preview(levels):
            nonlocal preview_levels
            preview_levels = levels

        # Instantiate dialog
        dialog = LevelsDialog(self.imageItem.levels, image_range,
                              self.imageItem.auto_levels,
                              limits=[min_limit, max_limit])
        dialog.levelsPreview.connect(preview)

        # Check the min and max protection
        assert dialog.min_spinbox.minimum() == min_limit
        assert dialog.min_spinbox.maximum() == max_limit
        assert dialog.max_spinbox.minimum() == min_limit
        assert dialog.max_spinbox.maximum() == max_limit

        # Change the values then accept the dialog
        input_levels = [-50, 50]
        dialog.automatic_checkbox.setChecked(False)
        dialog.min_spinbox.setValue(input_levels[1])
        dialog.max_spinbox.setValue(input_levels[0])
        assert preview_levels == [-50, 50]
        dialog.accept()

        # Check if changes are saved
        np.testing.assert_array_equal(dialog.levels, input_levels)


def test_get_decimals():
    assert get_decimals([-10.0, 1.0]) == 1
    assert get_decimals([-10.0, 10.0]) == 1
    assert get_decimals([1e-4, 1e-5]) == 7
    assert get_decimals([1e4, 1e-5]) == 1
    assert get_decimals([0.021, 0.01]) == 4
    assert get_decimals([0.0, 0.01]) == 1
    assert get_decimals([1e4, 1e5]) == 1
    assert get_decimals([0, 10.0]) == 1
    assert get_decimals([-10.0, 0.0]) == 1
