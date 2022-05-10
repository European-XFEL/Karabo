from unittest import mock

from karabogui import messagebox
from karabogui.binding.api import FloatBinding
from karabogui.controllers import icons_dialogs
from karabogui.testing import GuiTestCase, click_button


class TestChangeIconDialog(GuiTestCase):
    def test_order_of_items(self):
        """Test the order of items added.They should always be sorted.
        """
        dialog = icons_dialogs.DigitDialog([], FloatBinding())
        dialog.lessEqual.setChecked(True)

        for val in (10.0, 0.0, 20.0):
            dialog.value.setValue(val)
            click_button(dialog.addValue)
        valueList = dialog.valueList
        items = [valueList.item(i).text() for i in range(valueList.count())]
        assert items == ['<= 0.0', '<= 10.0', '<= 20.0']

        # Add existing value with different operator - '<'
        dialog.less.setChecked(True)
        dialog.value.setValue(10.0)
        click_button(dialog.addValue)
        items = [valueList.item(i).text() for i in range(valueList.count())]
        assert items == ['<= 0.0', '< 10.0', '<= 10.0', '<= 20.0']

        # Add existing value with same operator.
        with mock.patch.object(messagebox, 'show_error') as mock_error:
            click_button(dialog.addValue)
            message = "Cannot add new condition; it already exists."
            mock_error.assert_called_once_with(message, parent=dialog)
