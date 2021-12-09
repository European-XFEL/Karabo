from qtpy.QtWidgets import QSlider, QSpinBox

from karabogui.testing import GuiTestCase
from karabogui.util import SignalBlocker, _get_invalid_chars


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
