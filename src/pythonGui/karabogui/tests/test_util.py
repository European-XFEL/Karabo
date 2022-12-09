from unittest import main

from qtpy.QtWidgets import QSlider, QSpinBox

from karabogui.testing import GuiTestCase
from karabogui.util import (
    SignalBlocker, _get_invalid_chars, create_list_string, create_table_string,
    qtversion_compatible, version_compatible)


class TestUtilsGUI(GuiTestCase):

    def test_assert_filename(self):
        def assert_filename(filename, *, invalid):
            invalid_chars = _get_invalid_chars(filename)
            self.assertEqual(set(list(invalid)), set(invalid_chars))

        assert_filename("foo", invalid='')
        assert_filename("FoO/Bar-bAz_123", invalid='')
        assert_filename("foo bar", invalid=' ')
        assert_filename("foo!@#$%&*()+<>@:", invalid='!@#$%&*()+<>:')

    def test_signal_blocker(self):
        count = 0

        def slotReceived(*args, **kwargs):
            nonlocal count
            count += 1

        spinbox = QSpinBox()
        spinbox.valueChanged.connect(slotReceived)
        self.assertEqual(count, 0)
        spinbox.setValue(100)
        self.assertEqual(count, 1)

        # Update the spinbox, but block the signals
        with SignalBlocker(spinbox):
            for i in range(10):
                spinbox.setValue(i)
            self.assertEqual(spinbox.value(), i)

        # Still only once
        self.assertEqual(count, 1)
        spinbox.setValue(100)
        self.assertEqual(count, 2)

        # Reset the counter and setup a slider
        count = 0

        slider = QSlider()
        slider.valueChanged.connect(slotReceived)
        slider.setRange(0, 100)
        slider.setValue(20)
        self.assertEqual(count, 1)

        # Works, now block both
        with SignalBlocker(spinbox, slider):
            for i in range(10):
                spinbox.setValue(i)
                slider.setValue(i)
            self.assertEqual(spinbox.value(), i)
            self.assertEqual(slider.value(), i)
        self.assertEqual(count, 1)

        # Other initialization
        count = 0
        blocker = SignalBlocker(spinbox)
        self.assertEqual(len(blocker.objects), 1)
        with blocker:
            spinbox.setValue(10000)
        self.assertEqual(count, 0)

    def test_version_compatible(self):
        """Test that we can verify versions"""
        self.assertTrue(version_compatible("2.14.0a2", 2, 14))
        self.assertTrue(version_compatible("2.14.0a2", 2, 13))
        self.assertTrue(version_compatible("2.14.0rc1", 2, 14))
        self.assertFalse(version_compatible("2.14.0a2", 2, 15))
        self.assertTrue(version_compatible("asdeaeddevelopmode", 2, 14))
        self.assertTrue(version_compatible("asdeaeddevelopmode", 12, 33333))
        # minor smaller
        self.assertTrue(version_compatible("3.1.0a2", 2, 15))

        # Test garbage
        self.assertFalse(version_compatible("2.&7as7.13", 2, 0))

    def test_qversion_compatible(self):
        self.assertTrue(qtversion_compatible(5, 13))
        self.assertTrue(qtversion_compatible(5, 9))

    def test_html_list_string(self):
        info = ["1", "2", 4, []]
        html = create_list_string(info)
        assert html == "<ul><li>1</li><li>2</li><li>4</li><li>[]</li></ul>"

    def test_html_table_string(self):
        info = {"1": "a", 2: 3}
        html = create_table_string(info)
        assert html == "<table>" \
                       "<tr><td><b>1</b>:   </td><td>a</td></tr>" \
                       "<tr><td><b>2</b>:   </td><td>3</td></tr>" \
                       "</table>"


if __name__ == "__main__":
    main()
