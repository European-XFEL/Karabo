from unittest import main, mock

import pytest
from qtpy.QtCore import QItemSelectionModel

from karabogui.singletons.selection_tracker import SelectionTracker
from karabogui.testing import GuiTestCase


class TestProjectModel(GuiTestCase):
    def test_selection_tracker(self):
        """Test the Selection Tracker singleton"""
        tracker = SelectionTracker()

        model = mock.Mock()
        # Must be a `QItemSelectionModel`
        with pytest.raises(AssertionError):
            tracker.grab_selection(model)
        assert tracker._model is None

        model = QItemSelectionModel()
        model.clear = mock.Mock()
        tracker.grab_selection(model)
        assert tracker._model is model

        model.clear.assert_not_called()
        new_model = QItemSelectionModel()
        new_model.clear = mock.Mock()
        tracker.grab_selection(new_model)
        model.clear.assert_called_once()
        # Grabbing the selection only resets once the old model
        tracker.grab_selection(new_model)
        model.clear.assert_called_once()

        # New model is not reset!
        new_model.clear.assert_not_called()
        assert tracker._model is new_model


if __name__ == "__main__":
    main()
