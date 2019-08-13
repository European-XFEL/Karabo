from PyQt4.QtCore import QRectF

from karabo.common.scenemodel.api import LabelModel
from karabo.common.scenemodel.tests.utils import single_model_round_trip
from karabogui.testing import GuiTestCase

from ..clipboard import _add_models_to_clipboard, _read_models_from_clipboard


class TestClipboard(GuiTestCase):
    def setUp(self):
        super(TestClipboard, self).setUp()
        self.label_model = self._generate_label_model()

    def test_labelmodel_text(self):
        self._assert_label_text("bar")
        self._assert_label_text("90°")

    def _assert_label_text(self, text):
        # Chanage model text
        self.label_model.text = text
        read_model = single_model_round_trip(self.label_model)

        # Mock copy to clipboard
        _add_models_to_clipboard([read_model], QRectF(0, 0, 0, 0))
        copied_models = _read_models_from_clipboard()
        self.assertEqual(len(copied_models), 1)

        # Checked copied model has same traits as read model
        copied_model = copied_models[0]
        self.assertIsInstance(copied_model, LabelModel)
        self.assertEqual(read_model.get(), copied_model.get())

    def _generate_label_model(self):
        UBUNTU_FONT_SPEC = 'Ubuntu,48,-1,5,63,0,0,0,0,0'
        traits = {'x': 0, 'y': 0, 'height': 100, 'width': 100,
                  'text': 'foo', 'font': UBUNTU_FONT_SPEC,
                  'foreground': '#000000', 'background': '#ffffff',
                  'frame_width': 0}
        read_model = single_model_round_trip(LabelModel(**traits))
        return read_model
