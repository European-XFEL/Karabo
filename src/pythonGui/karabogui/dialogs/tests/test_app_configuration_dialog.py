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
from qtpy.QtCore import Qt

from karabogui.dialogs.api import ApplicationConfigurationDialog
from karabogui.singletons.configuration import Configuration
from karabogui.testing import GuiTestCase, singletons


class TestAppConfDialog(GuiTestCase):

    def test_basic_dialog(self):
        config = Configuration()
        with singletons(configuration=config):
            dialog = ApplicationConfigurationDialog()
            assert not dialog.isModal()
            model = dialog.tree_view.model()
            assert model is not None
            # We have 6 childen on root (groups)
            assert model.rowCount() == 5
            group_index = model.index(1, 0)
            index = model.index(0, 0, group_index)
            assert index.data() is not None
            index = model.index(0, 1, group_index)
            assert index.data() is not None

            assert model.headerData(
                0, Qt.Horizontal, Qt.DisplayRole) == 'Name'
            assert model.headerData(
                1, Qt.Horizontal, Qt.DisplayRole) == 'Setting'

            flag = model.flags(index)
            assert int(flag) == 33
            assert flag & Qt.ItemIsEnabled == Qt.ItemIsEnabled
            assert flag & Qt.ItemIsSelectable == Qt.ItemIsSelectable
            assert flag & Qt.ItemIsEditable != Qt.ItemIsEditable
            assert model.columnCount(None) == 2

            assert model.setData(index, "Karabo", Qt.EditRole)
            assert index.data() == "Karabo"

            # Check a boolean
            group_index = model.index(4, 0)
            assert group_index.data() == "user"
            index = model.index(9, 0, group_index)
            assert index.data() == "wizard"
            index_value = model.index(9, 1, group_index)
            flag = model.flags(index_value)
            assert int(flag) == 49
            assert flag & Qt.ItemIsEnabled == Qt.ItemIsEnabled
            assert flag & Qt.ItemIsUserCheckable == Qt.ItemIsUserCheckable

            assert not dialog.expanded
            dialog.expandAll()
            assert dialog.expanded

            # Double clicks, unfortunately, we cannot use QTest for that
            dialog.onDoubleClickHeader()
            assert not dialog.expanded
            dialog.onDoubleClickHeader()
            assert dialog.expanded

    def test_model_tester(self):
        from pytestqt.modeltest import ModelTester
        config = Configuration()
        with singletons(configuration=config):
            dialog = ApplicationConfigurationDialog()
            model = dialog.tree_view.model()
            tester = ModelTester(None)
            tester.check(model)
