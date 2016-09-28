from ..widgets.plot import PlotCurveModel, LinePlotModel
from .utils import (assert_base_traits, base_widget_traits,
                    single_model_round_trip)


def test_line_plot_widget():
    curve = PlotCurveModel(device='dev1', path='prop', curve_object_data='00')
    for name in ('DisplayTrendline', 'XYVector'):
        traits = base_widget_traits(parent='DisplayComponent')
        traits['klass'] = name
        traits['boxes'] = [curve]
        model = LinePlotModel(**traits)
        read_model = single_model_round_trip(model)
        assert_base_traits(read_model)
        assert read_model.klass == name
        assert len(read_model.boxes) == 1
        assert read_model.boxes[0].device == 'dev1'
        assert read_model.boxes[0].path == 'prop'
        assert read_model.boxes[0].curve_object_data == '00'


def test_line_plot_duplicate_curves():
    curve0 = PlotCurveModel(device='dev1', path='foo', curve_object_data='00')
    curve1 = PlotCurveModel(device='dev1', path='bar', curve_object_data='11')
    curve2 = PlotCurveModel(device='dev1', path='foo', curve_object_data='22')
    traits = base_widget_traits(parent='DisplayComponent')
    model = LinePlotModel(klass='XYVector', boxes=[curve0, curve1], **traits)

    model.boxes.append(curve1)
    model.boxes.append(curve2)
    assert len(model.boxes) == 2
    assert model.boxes[0].curve_object_data == curve2.curve_object_data
    assert model.boxes[1] is curve1
