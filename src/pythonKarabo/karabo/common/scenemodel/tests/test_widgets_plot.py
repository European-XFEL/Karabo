from io import StringIO

from ..api import PlotCurveModel, LinePlotModel, SparklineModel, read_scene
from .utils import (assert_base_traits, base_widget_traits,
                    single_model_round_trip)

OLD_SPARKY = """
<svg:svg
    xmlns:krb="http://karabo.eu/scene"
    xmlns:svg="http://www.w3.org/2000/svg"
    height="540" width="728"
    krb:version="2" >
    <svg:rect
        height="364" width="426" x="256" y="172"
        krb:class="DisplayComponent"
        krb:keys="thePast.sparkProp"
        krb:widget="DisplaySparkline" >
        <krb:box device="thePast" path="sparkProp">BINARYBLOB</krb:box>
    </svg:rect>
</svg:svg>
"""


def test_line_plot_widget():
    curve = PlotCurveModel(device='dev1', path='prop', curve_object_data='00')
    for name in ('DisplayTrendline', 'XYVector'):
        traits = base_widget_traits()
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
    traits = base_widget_traits()
    model = LinePlotModel(klass='XYVector', boxes=[curve0], **traits)

    model.add_curve(curve1)
    model.add_curve(curve2)
    assert len(model.boxes) == 2
    assert model.boxes[0].curve_object_data == curve2.curve_object_data
    assert model.boxes[1] is curve1


def test_sparkline_basics():
    traits = base_widget_traits()
    traits['time_base'] = 42
    traits['show_value'] = True
    traits['show_format'] = 'whatever'
    model = SparklineModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert model.time_base == 42
    assert model.show_value
    assert model.show_format == 'whatever'


def test_old_sparkline_data():
    default = SparklineModel()
    with StringIO(OLD_SPARKY) as fp:
        scene = read_scene(fp)

    model = scene.children[0]
    assert isinstance(model, SparklineModel)
    assert model.keys == ['thePast.sparkProp']
    assert model.time_base == default.time_base
    assert model.show_value == default.show_value
    assert model.show_format == default.show_format
