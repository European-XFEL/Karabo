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
import pytest
from qtpy.QtCore import QItemSelectionModel

from karabogui.singletons.selection_tracker import SelectionTracker


def test_selection_tracker(mocker):
    """Test the Selection Tracker singleton"""
    tracker = SelectionTracker()

    model = mocker.Mock()
    # Must be a `QItemSelectionModel`
    with pytest.raises(AssertionError):
        tracker.grab_selection(model)
    assert tracker._model is None

    model = QItemSelectionModel()
    model.clear = mocker.Mock()
    tracker.grab_selection(model)
    assert tracker._model is model

    model.clear.assert_not_called()
    new_model = QItemSelectionModel()
    new_model.clear = mocker.Mock()
    tracker.grab_selection(new_model)
    model.clear.assert_called_once()
    # Grabbing the selection only resets once the old model
    tracker.grab_selection(new_model)
    model.clear.assert_called_once()

    # New model is not reset!
    new_model.clear.assert_not_called()
    assert tracker._model is new_model


if __name__ == "__main__":
    pytest.main()
