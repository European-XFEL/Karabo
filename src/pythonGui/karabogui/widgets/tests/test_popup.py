from karabogui.testing import GuiTestCase
from karabogui.widgets.popup import PopupWidget


class TestPopUp(GuiTestCase):

    def test_default_widget(self):
        widget = PopupWidget()
        # Freeze is `False` as default
        layout = widget.layout()
        self.assertFalse(layout.isEmpty())
        self.assertIsNone(getattr(widget, '_ui_freeze_button', None))
        self.assertEqual(widget.freeze, False)

        # Test the info setting
        info = {"minInc": 5, "description": "This is a description"}
        widget.setInfo(info)
        plain_text = "\nminInc: \n5\ndescription: \nThis is a description\n"
        self.assertEqual(widget.text, plain_text)
        widget.close()

    def test_freeze_widget(self):
        widget = PopupWidget(can_freeze=True)
        layout = widget.layout()
        self.assertFalse(layout.isEmpty())
        self.assertIsNotNone(getattr(widget, '_ui_freeze_button', None))
        self.assertEqual(widget.freeze, False)

        # Test the info setting
        info = {"minInc": 20, "description": "This is freeze"}
        widget.setInfo(info)
        plain_text = "\nminInc: \n20\ndescription: \nThis is freeze\n"
        self.assertEqual(widget.text, plain_text)

        # Toggle freeze and test the caching
        widget.toggle_freeze()
        self.assertEqual(widget.freeze, True)
        # Test the info setting
        info = {"alarmLow": 20, "displayedName": "Alarm Info"}
        widget.setInfo(info)
        # Text did not change... But if we toggle, we change again!
        self.assertEqual(widget.text, plain_text)
        widget.toggle_freeze()
        self.assertEqual(widget.freeze, False)
        plain_text = "\nalarmLow: \n20\ndisplayedName: \nAlarm Info\n"
        self.assertEqual(widget.text, plain_text)

        # Test the reset
        widget.toggle_freeze()
        self.assertEqual(widget.freeze, True)
        widget.reset()
        self.assertEqual(widget.freeze, False)
        self.assertEqual(widget.text, plain_text)
        widget.close()
