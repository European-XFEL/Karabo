from qtpy.QtCore import QRectF
from qtpy.QtWidgets import QBoxLayout

from karabo.common.scenemodel.api import (
    BoxLayoutModel, DisplayLabelModel, LabelModel)
from karabo.common.scenemodel.tests.utils import single_model_round_trip
from karabogui.testing import GuiTestCase

from ..clipboard import _add_models_to_clipboard, _read_models_from_clipboard


class TestClipboard(GuiTestCase):

    def test_copy_paste(self):
        """These are standalone widgets that are not bound to any devices."""
        # Test single model selection
        label_model = self._get_label_model("foo")
        self._assert_copy_paste(label_model)

        # Test single layout selection
        layout_model = self._get_layout_model(label_model)
        self._assert_copy_paste(layout_model)

        # Test multiple models selection
        another_label = self._get_label_model("bar", x=10, y=10)
        self._assert_copy_paste(label_model, another_label)

        # Test multiple layouts selection
        another_layout = self._get_layout_model(another_label, x=10, y=10)
        self._assert_copy_paste(layout_model, another_layout)

    def _assert_copy_paste(self, *models):
        # Do a single round trip for each models to verify their integrity
        read_models = [single_model_round_trip(model) for model in models]
        for read_model, model in zip(read_models, models):
            self._assert_model(read_model, model)

        # Mock copy to clipboard
        _add_models_to_clipboard(read_models, rect=QRectF(0, 0, 0, 0))
        copied_models = _read_models_from_clipboard()
        self.assertEqual(len(copied_models), len(models))

        # Checked copied model has same traits as read model
        for copied_model, read_model in zip(copied_models, read_models):
            self._assert_model(copied_model, read_model)

    def _assert_model(self, actual, expected):
        trait_names = expected.copyable_trait_names()
        if "children" in trait_names:
            trait_names.remove("children")
            iter_children = zip(actual.children, expected.children)
            for actual_child, expected_child in iter_children:
                self._assert_model(actual_child, expected_child)

        self.assertTrue(type(actual), type(expected))
        self.assertDictEqual(actual.trait_get(trait_names),
                             expected.trait_get(trait_names))

    @staticmethod
    def _get_label_model(text, x=0, y=0):
        UBUNTU_FONT_SPEC = 'Ubuntu,48,-1,5,63,0,0,0,0,0'
        traits = {'x': x, 'y': y, 'height': 100, 'width': 100,
                  'text': text, 'font': UBUNTU_FONT_SPEC,
                  'foreground': '#000000', 'background': '#ffffff',
                  'frame_width': 0}
        return LabelModel(**traits)

    @staticmethod
    def _get_layout_model(model, x=0, y=0):
        # Initialize children models
        height = 30
        label_model = LabelModel(width=100, height=height, text="bar")
        display_model = DisplayLabelModel(width=200, height=height)
        model.trait_set(width=300, height=height)
        children_model = [label_model, display_model, model]

        # Initialize layout model
        hbox_model = BoxLayoutModel(x=x, y=y, children=children_model,
                                    direction=QBoxLayout.LeftToRight)

        # Correct the position from the initial position and width
        x0, y0 = (x, y)
        for child in children_model:
            child.trait_set(x=x0, y=y0)
            # Add child width for the next iteration
            x0 += child.width
        hbox_model.height = height
        hbox_model.width = sum([child.width for child in children_model])
        return hbox_model
