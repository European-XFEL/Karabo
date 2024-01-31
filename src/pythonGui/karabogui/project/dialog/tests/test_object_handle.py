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
from qtpy.QtGui import QValidator

from karabogui.testing import GuiTestCase

from ..object_handle import ObjectDuplicateDialog


class TestObjectDuplicateDialog(GuiTestCase):
    TEXT = 'You are about to create <b>{}</b> duplicate(s)'

    def setUp(self):
        super().setUp()
        simple_name = "foo"
        self.dialog = ObjectDuplicateDialog(simple_name)

    def test_default_values(self):
        start = 0
        end = 0
        assert self.dialog.sbStart.value() == start
        assert self.dialog.sbEnd.value() == end
        assert self.dialog.laText.text() == (
               self.TEXT.format(self._num_duplicates(start, end)))

    def test_set_higher_end_value(self):
        start = 0
        end = 10
        self.dialog.sbEnd.setValue(end)

        assert self.dialog.sbStart.value() == start
        assert self.dialog.sbEnd.value() == end
        assert self.dialog.laText.text() == \
               self.TEXT.format(self._num_duplicates(start, end))

    def test_set_higher_start_value(self):
        start = 10
        self.dialog.sbStart.setValue(start)

        assert self.dialog.sbStart.value() == start
        assert self.dialog.sbEnd.value() == start
        assert self.dialog.laText.text() == \
               self.TEXT.format(self._num_duplicates(start, start))

    def test_input_validity(self):
        self._assert_input("100", is_valid=True)
        self._assert_input("-1", is_valid=False)  # negative
        self._assert_input("1001", is_valid=False)  # exceeding
        self._assert_input("1,000", is_valid=False)  # comma-separated
        self._assert_input("1.00", is_valid=False)  # decimals

    def _assert_input(self, value, *, is_valid):
        valid = QValidator.Acceptable if is_valid else QValidator.Invalid
        pos = 0

        start_result, *_ = self.dialog.sbStart.validate(value, pos)
        assert start_result == valid
        end_result, *_ = self.dialog.sbEnd.validate(value, pos)
        assert end_result == valid

    def _num_duplicates(self, start, end):
        return (end - start) + 1
