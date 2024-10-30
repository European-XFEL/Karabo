import pytest
from traits.api import Instance, List, String

from karabo.common.scenemodel.api import (
    BaseROIData, BaseWidgetObjectData, CrossROIData, RectROIData, SceneModel)
from karabo.interactive import scene2python


class SimpleModel(BaseWidgetObjectData):
    name = String
    rois = List(Instance(BaseROIData))


class MultiChildren(BaseWidgetObjectData):
    name = String
    cross_rois = List(Instance(CrossROIData))
    rect_rois = List(Instance(RectROIData))


@pytest.fixture
def models():
    cross_roi1 = CrossROIData(name="FirstCrossROI", x=10, y=10)
    cross_roi2 = CrossROIData(name="SecondCrossROI", x=2, y=7)

    rect_roi1 = RectROIData(name="FirstRectROI", x=6, y=21)
    rect_roi2 = RectROIData(name="SecondRectROI", x=12, y=43)

    rois = [cross_roi1, cross_roi2, rect_roi1, rect_roi2]
    cross_rois = [cross_roi1, cross_roi2]
    rect_rois = [rect_roi1, rect_roi2]

    # Single Children
    simple = SimpleModel(name="only_one_child", rois=rois)

    # Mulitple children (More than one list trait)
    multi_child = MultiChildren(name="multiple_children",
                                cross_rois=cross_rois,
                                rect_rois=rect_rois)
    return simple, multi_child


def test_code_from_simple_model(models):
    simple, multi_child = models
    classes, code = scene2python.convert_scene_model_to_code(simple)
    assert classes == {"CrossROIData", "RectROIData", "SimpleModel"}
    expected_code = [
        "scene00 = CrossROIData(name='FirstCrossROI', x=10.0, y=10.0)",
        "scene01 = CrossROIData(name='SecondCrossROI', x=2.0, y=7.0)",
        "scene02 = RectROIData(name='FirstRectROI', x=6.0, y=21.0)",
        "scene03 = RectROIData(name='SecondRectROI', x=12.0, y=43.0)",
        ("scene = SimpleModel(name='only_one_child', "
         "parent_component='DisplayComponent', rois=[scene00, scene01, "
         "scene02, scene03])")
    ]
    assert code == expected_code

    # With Multiple Children
    classes, complex_code = scene2python.convert_scene_model_to_code(
        multi_child)
    assert classes == {"CrossROIData", "RectROIData", "MultiChildren"}
    expected_code = [
        "scene00 = CrossROIData(name='FirstCrossROI', x=10.0, y=10.0)",
        "scene01 = CrossROIData(name='SecondCrossROI', x=2.0, y=7.0)",
        "scene10 = RectROIData(name='FirstRectROI', x=6.0, y=21.0)",
        "scene11 = RectROIData(name='SecondRectROI', x=12.0, y=43.0)",
        ("scene = MultiChildren(name='multiple_children', "
         "parent_component='DisplayComponent', cross_rois=[scene00, "
         "scene01], rect_rois=[scene10, scene11])")
    ]

    assert expected_code == complex_code


def test_scene_model(models):
    # Within a Scene model.
    simple, multi_child = models
    simple_scene_model = SceneModel(children=[simple])
    classes, code = scene2python.convert_scene_model_to_code(
        simple_scene_model)
    assert classes == {'CrossROIData', 'RectROIData', 'SceneModel',
                       'SimpleModel'}
    expected = [
        "scene0000 = CrossROIData(name='FirstCrossROI', x=10.0, y=10.0)",
        "scene0001 = CrossROIData(name='SecondCrossROI', x=2.0, y=7.0)",
        "scene0002 = RectROIData(name='FirstRectROI', x=6.0, y=21.0)",
        "scene0003 = RectROIData(name='SecondRectROI', x=12.0, y=43.0)",
        ("scene00 = SimpleModel(name='only_one_child', "
         "parent_component='DisplayComponent', rois=[scene0000, scene0001, "
         "scene0002, scene0003])"),
        'scene = SceneModel(children=[scene00])']
    assert code == expected
