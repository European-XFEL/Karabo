import base64
import pickle

from karabo_gui.displaywidgets.displaytrendline import DisplayTrendline, Curve
from karabo_gui.displaywidgets.xyvectors import XYVector
from karabo_gui.scenemodel.api import PlotCurveModel
from karabo_gui.topology import getDevice
from .base import BaseWidgetContainer


def _decode_plot_curve_model(model):
    """ Turn a PlotCurveModel instance into a box and a curve.
    """
    box = getDevice(model.device).getBox(model.path.split("."))
    curve = pickle.loads(base64.b64decode(model.curve_object_data))
    return box, curve


def _encode_plot_curve_model(box, curve):
    """ Turn box and a curve into a PlotCurveModel
    """
    device_id = box.configuration.id
    path = ".".join(box.path)
    curve = base64.b64encode(pickle.dumps(curve)).decode("ascii")
    return PlotCurveModel(device=device_id, path=path, curve_object_data=curve)


class _LinePlotWrapperMixin(object):
    def __init__(self, model, box, parent):
        self.model = model  # Set the model first! `_addCurve` may be called.
        super(_LinePlotWrapperMixin, self).__init__(box, parent)

        # Initialize the widget
        for plot in model.boxes:
            box, curve = _decode_plot_curve_model(plot)
            super(_LinePlotWrapperMixin, self)._addCurve(box, curve)

    def _addCurve(self, box, curve):
        super(_LinePlotWrapperMixin, self)._addCurve(box, curve)
        if isinstance(curve, Curve):
            curve = curve.curve  # Strip off some extraneous wrapping
        # The data model will handle duplicate curves!
        self.model.boxes.append(_encode_plot_curve_model(box, curve))


class _DisplayTrendlineWrapper(_LinePlotWrapperMixin, DisplayTrendline):
    """ A wrapper around DisplayTrendline
    """


class _XYVectorWrapper(_LinePlotWrapperMixin, XYVector):
    """ A wrapper around XYVector
    """


class LinePlotContainer(BaseWidgetContainer):
    def _create_widget(self, boxes):
        factories = {
            'DisplayTrendline': _DisplayTrendlineWrapper,
            'XYVector': _XYVectorWrapper,
        }
        factory = factories[self.model.klass]
        display_widget = factory(self.model, boxes[0], self)
        for b in boxes[1:]:
            display_widget.addBox(b)
        return display_widget
