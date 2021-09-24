from karabo.common.scenemodel.api import CrossROIData, RectROIData
from karabo.common.scenemodel.tests.utils import (
    base_widget_traits, single_model_round_trip)

from .. import api


def _geometry_traits():
    return {'x': 0, 'y': 0, 'height': 100, 'width': 100}


def _assert_geometry_traits(model):
    traits = _geometry_traits()
    for name, value in traits.items():
        msg = "{} has the wrong value!".format(name)
        assert getattr(model, name) == value, msg


def test_webcam_graph_model():
    traits = _geometry_traits()
    traits['colormap'] = 'inferno'
    model = api.WebCamGraphModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.colormap == 'inferno'


def test_detector_graph_model():
    traits = _geometry_traits()
    traits['colormap'] = 'magma'
    model = api.DetectorGraphModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.colormap == 'magma'


def test_vector_roll_graph_model():
    traits = _geometry_traits()
    traits['colormap'] = 'magma'
    traits['maxlen'] = 2000
    model = api.VectorRollGraphModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.colormap == 'magma'
    assert read_model.maxlen == 2000


def test_image_graph_model():
    roi_data = [
        RectROIData(**{'roi_type': 1, 'x': 0, 'y': 0, 'w': 500, 'h': 500}),
        CrossROIData(**{'roi_type': 2, 'x': 150, 'y': 150})
    ]

    traits = base_widget_traits()
    traits['aux_plots'] = 1
    traits['colormap'] = 'viridis'
    traits['roi_items'] = roi_data
    traits['x_scale'] = 2.0
    traits['x_translate'] = 3.1
    traits['x_label'] = 'N-X-axis'
    traits['x_units'] = 'mm'
    traits['y_scale'] = 13.0
    traits['y_translate'] = 2.1
    traits['y_label'] = 'N-Y-axis'
    traits['y_units'] = 'nm'
    traits['show_scale'] = False

    model = api.ImageGraphModel(**traits)
    read_model = single_model_round_trip(model)

    assert read_model.aux_plots == 1
    assert read_model.colormap == 'viridis'

    assert read_model.x_scale == 2.0
    assert read_model.x_translate == 3.1
    assert read_model.x_label == 'N-X-axis'
    assert read_model.x_units == 'mm'

    assert read_model.y_scale == 13.0
    assert read_model.y_translate == 2.1
    assert read_model.y_label == 'N-Y-axis'
    assert read_model.y_units == 'nm'
    assert read_model.show_scale is False

    # Assert ROI data
    for orig, read in zip(model.roi_items, read_model.roi_items):
        for trait in orig.class_visible_traits():
            assert getattr(orig, trait) == getattr(read, trait)
