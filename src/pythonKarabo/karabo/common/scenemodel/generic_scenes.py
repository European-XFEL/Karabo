from .model import SceneModel
from .shapes import LineModel
from .widgets.plot import LinePlotModel
from .widgets.simple import DisplayLabelModel, LabelModel


def get_trendline_scene(device_id, path):
    """Return a generic trendline scene for an device_id with path

    :returns: SceneModel with a trendline and a deviceId label
    """

    line = LineModel(
        stroke='#000000', x1=15.0, x2=615.0, y1=75.0, y2=75.0)

    plot = LinePlotModel(
        height=337.0,
        keys=['{}.{}'.format(device_id, path)],
        parent_component='DisplayComponent', width=600.0,
        x=10.0, y=80.0)

    label = LabelModel(
        font='Ubuntu,11,-1,5,50,0,0,0,0,0',
        foreground='#000000', height=31.0,
        parent_component='DisplayComponent', text='DeviceID',
        width=60.0, x=20.0, y=20.0)

    display_label = DisplayLabelModel(
        height=30.0,
        keys=['{}.deviceId'.format(device_id)],
        parent_component='DisplayComponent',
        width=540.0, x=80.0, y=20.0)

    scene = SceneModel(height=440.0, width=630.0,
                       children=[line, plot, label, display_label])

    scene.simple_name = 'Trendline_{}_{}'.format(device_id, path)

    return scene
