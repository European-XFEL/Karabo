# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from karabo.common.scenemodel.api import CrossROIData
from karabo.common.scenemodel.tests.utils import single_model_round_trip
from karabo.common.scenemodel.widgets.graph_utils import CurveOptions

from .. import api


def _geometry_traits():
    return {"x": 0, "y": 0, "height": 100, "width": 100}


def _assert_geometry_traits(model):
    traits = _geometry_traits()
    for name, value in traits.items():
        msg = f"{name} has the wrong value!"
        assert getattr(model, name) == value, msg


def test_base_plot_model():
    traits = _geometry_traits()
    traits["title"] = "Graph"
    traits["background"] = "white"
    traits["x_label"] = "X"
    traits["y_label"] = "Y"
    traits["x_units"] = "XUNIT"
    traits["y_units"] = "YUNIT"
    traits["x_grid"] = True
    traits["y_grid"] = False
    traits["x_log"] = True
    traits["y_log"] = False
    traits["x_min"] = 0.1
    traits["x_max"] = 12.0
    traits["y_min"] = 0.2
    traits["y_max"] = 14.0
    traits["x_invert"] = True
    traits["y_invert"] = True
    traits["x_autorange"] = False
    traits["y_autorange"] = False
    model = api.VectorGraphModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.title == "Graph"
    assert read_model.background == "white"
    assert read_model.x_label == "X"
    assert read_model.y_label == "Y"
    assert read_model.x_units == "XUNIT"
    assert read_model.y_units == "YUNIT"
    assert read_model.x_autorange is False
    assert read_model.y_autorange is False
    assert read_model.x_grid is True
    assert read_model.y_grid is False
    assert read_model.x_log is True
    assert read_model.y_log is False
    assert read_model.x_min == 0.1
    assert read_model.x_max == 12.0
    assert read_model.y_min == 0.2
    assert read_model.y_max == 14.0
    assert read_model.x_invert is True
    assert read_model.y_invert is True


def test_scatter_graph_model():
    traits = _geometry_traits()
    traits["maxlen"] = 2000
    traits["psize"] = 4.3
    model = api.ScatterGraphModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.maxlen == 2000
    assert read_model.psize == 4.3


def test_vector_bar_graph_model():
    traits = _geometry_traits()
    traits["bar_width"] = 5.7
    model = api.VectorBarGraphModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.bar_width == 5.7


def test_vector_hist_graph_model():
    traits = _geometry_traits()
    traits["bins"] = 23
    traits["auto"] = False
    traits["start"] = 0.1
    traits["stop"] = 10.0
    model = api.VectorHistGraphModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.bins == 23
    assert read_model.start == 0.1
    assert read_model.stop == 10.0
    assert read_model.auto is False


def test_vector_graph():
    traits = _geometry_traits()
    roi_data = [
        CrossROIData(**{"roi_type": 2, "x": 150, "y": 150, "name": "Cross 1"}),
        CrossROIData(**{"roi_type": 2, "x": 150, "y": 150, "name": "Cross 2"}),
    ]
    traits["half_samples"] = 10000
    traits["roi_items"] = roi_data
    traits["roi_tool"] = 1
    traits["offset"] = 15.0
    traits["step"] = 17.0
    curve_options = [
        CurveOptions(**{"key": "first_plot",
                        "pen_color": "#ff7f00",
                        "legend_name": "Curve 1",
                        "plot_type": 1}),
        CurveOptions(**{"key": "second_plot",
                        "pen_color": "#fb9a99",
                        "legend_name": "Curve 2",
                        "plot_type": 1})]
    traits["curve_options"] = curve_options

    model = api.VectorGraphModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    # Assert ROI data
    for orig, read in zip(model.roi_items, read_model.roi_items):
        for trait in orig.copyable_trait_names():
            assert getattr(orig, trait) == getattr(read, trait)

    assert read_model.half_samples == 10000
    assert read_model.roi_tool == 1
    assert read_model.offset == 15.0
    assert read_model.step == 17.0

    for orig, read in zip(model.curve_options, read_model.curve_options):
        for trait in orig.copyable_trait_names():
            assert getattr(orig, trait) == getattr(read, trait)


def test_vector_scatter_graph():
    traits = _geometry_traits()
    traits["psize"] = 3.2
    model = api.VectorScatterGraphModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)
    assert read_model.psize == 3.2


def test_ndarray_graph():
    traits = _geometry_traits()
    roi_data = [
        CrossROIData(**{"roi_type": 2, "x": 150, "y": 150, "name": "Cross 1"}),
        CrossROIData(**{"roi_type": 2, "x": 150, "y": 150, "name": "Cross 2"}),
    ]
    traits["half_samples"] = 10000
    traits["roi_items"] = roi_data
    traits["roi_tool"] = 1
    traits["offset"] = 5.0
    traits["step"] = 7.0

    model = api.NDArrayGraphModel(**traits)
    read_model = single_model_round_trip(model)
    _assert_geometry_traits(read_model)

    # Assert ROI data
    for orig, read in zip(model.roi_items, read_model.roi_items):
        for trait in orig.copyable_trait_names():
            assert getattr(orig, trait) == getattr(read, trait)

    assert read_model.half_samples == 10000
    assert read_model.roi_tool == 1
    assert read_model.offset == 5.0
    assert read_model.step == 7.0
