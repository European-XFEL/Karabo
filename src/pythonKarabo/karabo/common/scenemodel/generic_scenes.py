from .model import SceneModel
from .shapes import LineModel
from .widgets.graph_image import WebCamGraphModel
from .widgets.graph_plots import TrendGraphModel, VectorGraphModel
from .widgets.simple import DisplayLabelModel, LabelModel


def get_trendline_scene(device_id, path):
    """Return a generic trendline scene for an device_id with path

    :returns: SceneModel with a trendline and a deviceId label
    """

    line = LineModel(
        stroke='#000000', x1=15.0, x2=615.0, y1=75.0, y2=75.0)

    plot = TrendGraphModel(
        height=337.0,
        keys=['{}.{}'.format(device_id, path)],
        x_label="Time Axis",
        y_label=path,
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

    scene.simple_name = 'TrendGraph_{}_{}'.format(device_id, path)

    return scene


def get_image_scene(device_id, path):
    """Return a webcam graph scene for an device_id with path

    :returns: SceneModel with a webcam graph and a deviceId label
    """

    line = LineModel(
        stroke='#000000', x1=15.0, x2=615.0, y1=75.0, y2=75.0)

    image = WebCamGraphModel(
        colormap="viridis",
        height=384.0,
        keys=['{}.{}'.format(device_id, path)],
        parent_component='DisplayComponent',
        width=603.0, x=15.0, y=83.0)

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

    scene = SceneModel(height=470.0, width=630.0,
                       children=[line, image, label, display_label])

    scene.simple_name = 'Image_{}_{}'.format(device_id, path)

    return scene


def get_vector_scene(device_id, path):
    """Return a vector graph scene for an device_id with path

    :returns: SceneModel with a vector graph and a deviceId label
    """

    line = LineModel(
        stroke='#000000', x1=15.0, x2=615.0, y1=75.0, y2=75.0)

    image = VectorGraphModel(
        x_grid=True,
        y_grid=True,
        height=384.0,
        keys=['{}.{}'.format(device_id, path)],
        parent_component='DisplayComponent',
        width=603.0, x=15.0, y=83.0)

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

    scene = SceneModel(height=470.0, width=630.0,
                       children=[line, image, label, display_label])

    scene.simple_name = 'Image_{}_{}'.format(device_id, path)

    return scene
