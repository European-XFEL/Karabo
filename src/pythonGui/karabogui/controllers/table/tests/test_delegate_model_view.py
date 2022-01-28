from unittest import main

from karabo.common.scenemodel.api import TableElementModel
from karabo.native import (
    AccessMode, Bool, Configurable, Double, Hash, Int32, String, VectorHash)
from karabogui.binding.config import apply_configuration
from karabogui.controllers.table.api import (
    BaseTableController, BoolButtonDelegate, ColorBindingDelegate,
    ColorNumberDelegate, ProgressBarDelegate)
from karabogui.testing import GuiTestCase, get_property_proxy


class TableSchema(Configurable):
    boolButton = Bool(
        defaultValue=False,
        displayType="TableBoolButton",
        displayedName="Bool Button",
        accessMode=AccessMode.READONLY)
    colorText = String(
        displayType="TableColor|default=red&ginger=blue&bailey=green",
        displayedName="Color Text",
        defaultValue="Indiana",
        accessMode=AccessMode.READONLY)
    colorNumber = Int32(
        defaultValue=1,
        displayType="TableColor|1=blue&2=red&default=black")
    progress = Double(
        displayedName="Progress",
        displayType="TableProgressBar",
        defaultValue=0.0,
        minInc=0.0,
        maxInc=100.0)


class Object(Configurable):
    prop = VectorHash(rows=TableSchema)


class TestDelegateModelView(GuiTestCase):
    def setUp(self):
        super().setUp()
        self.proxy = get_property_proxy(Object.getClassSchema(), "prop")
        self.model = TableElementModel()
        self.controller = BaseTableController(proxy=self.proxy,
                                              model=self.model)
        self.controller.create(None)
        self.assertTrue(self.controller.isReadOnly())
        self.controller.set_read_only(False)
        self.table_hash = Hash(
            "prop",
            [Hash("boolButton", True, "colorText", "ginger",
                  "colorNumber", 10, "progress", 75.0)])
        apply_configuration(self.table_hash, self.proxy.root_proxy.binding)

    def tearDown(self):
        self.controller.destroy()

    def test_has_delegates(self):
        widget = self.controller.tableWidget()
        delegate = widget.itemDelegateForColumn(0)
        self.assertNotIsInstance(delegate, BoolButtonDelegate)
        delegate = widget.itemDelegateForColumn(1)
        self.assertIsInstance(delegate, ColorBindingDelegate)
        delegate = widget.itemDelegateForColumn(2)
        self.assertIsInstance(delegate, ColorNumberDelegate)
        delegate = widget.itemDelegateForColumn(3)
        self.assertIsInstance(delegate, ProgressBarDelegate)

        self.controller.set_read_only(True)
        delegate = widget.itemDelegateForColumn(0)
        self.assertIsInstance(delegate, BoolButtonDelegate)


if __name__ == "__main__":
    main()
