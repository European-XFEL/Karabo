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
from .model import SceneModel
from .shapes import LineModel
from .widgets.graph_image import WebCamGraphModel
from .widgets.graph_plots import (
    AlarmGraphModel, StateGraphModel, TrendGraphModel, VectorGraphModel)
from .widgets.simple import DisplayLabelModel, HistoricTextModel, LabelModel


def _get_plot_scene(model, device_id, path):
    line = LineModel(stroke="#000000", x1=15.0, x2=700.0, y1=65.0, y2=65.0)

    plot = model(
        height=337.0,
        keys=[f"{device_id}.{path}"],
        y_label=path,
        x_grid=True,
        y_grid=True,
        parent_component="DisplayComponent",
        width=700.0,
        x=10.0,
        y=80.0,
    )

    label = LabelModel(
        font="Source Sans Pro,10,-1,5,50,0,0,0,0,0",
        foreground="#000000",
        height=31.0,
        parent_component="DisplayComponent",
        text="DeviceID",
        width=60.0,
        x=20.0,
        y=20.0,
    )

    display_label = DisplayLabelModel(
        height=30.0,
        keys=[f"{device_id}.deviceId"],
        parent_component="DisplayComponent",
        width=600.0,
        x=80.0,
        y=20.0,
    )

    scene = SceneModel(
        height=440.0, width=730.0, children=[line, plot, label, display_label]
    )

    return scene


def get_text_history_scene(device_id, path):
    """Return a history scene for a device_id with string path

    :returns: SceneModel with a historic table and a deviceId label
    """
    line = LineModel(stroke="#000000", x1=15.0, x2=700.0, y1=65.0, y2=65.0)

    text = HistoricTextModel(
        height=600.0,
        keys=[f"{device_id}.{path}"],
        parent_component="DisplayComponent",
        width=700.0,
        x=10.0,
        y=80.0,
    )

    label = LabelModel(
        font="Source Sans Pro,10,-1,5,50,0,0,0,0,0",
        foreground="#000000",
        height=31.0,
        parent_component="DisplayComponent",
        text="DeviceID",
        width=60.0,
        x=20.0,
        y=20.0,
    )

    display_label = DisplayLabelModel(
        height=30.0,
        keys=[f"{device_id}.deviceId"],
        parent_component="DisplayComponent",
        width=600.0,
        x=80.0,
        y=20.0,
    )

    scene = SceneModel(
        height=700.0, width=730.0, children=[line, text, label, display_label]
    )
    scene.simple_name = f"HistoricText_{device_id}_{path}"

    return scene


def get_trendline_scene(device_id, path):
    """Return a generic trendline scene for a device_id with path

    :returns: SceneModel with a state graph trend and a deviceId label
    """
    scene = _get_plot_scene(TrendGraphModel, device_id, path)
    scene.simple_name = f"TrendGraph_{device_id}_{path}"

    return scene


def get_state_graph_scene(device_id, path):
    """Return a state graph scene for a device_id with path

    :returns: SceneModel with a trendline and a deviceId label
    """
    scene = _get_plot_scene(StateGraphModel, device_id, path)
    scene.simple_name = f"StateGraph_{device_id}_{path}"

    return scene


def get_alarm_graph_scene(device_id, path):
    """Return a alarm graph scene for a device_id with path

    :returns: SceneModel with a trendline and a deviceId label
    """
    scene = _get_plot_scene(AlarmGraphModel, device_id, path)
    scene.simple_name = f"AlarmGraph_{device_id}_{path}"

    return scene


def get_vector_scene(device_id, path):
    """Return a vector graph scene for a device_id with path

    :returns: SceneModel with a vector graph and a deviceId label
    """
    scene = _get_plot_scene(VectorGraphModel, device_id, path)
    scene.simple_name = f"Image_{device_id}_{path}"

    return scene


def get_image_scene(device_id, path):
    """Return a webcam graph scene for a device_id with path

    :returns: SceneModel with a webcam graph and a deviceId label
    """

    line = LineModel(stroke="#000000", x1=15.0, x2=615.0, y1=75.0, y2=75.0)

    image = WebCamGraphModel(
        colormap="viridis",
        height=384.0,
        keys=[f"{device_id}.{path}"],
        parent_component="DisplayComponent",
        width=603.0,
        x=15.0,
        y=83.0,
    )

    label = LabelModel(
        font="Source Sans Pro,10,-1,5,50,0,0,0,0,0",
        foreground="#000000",
        height=31.0,
        parent_component="DisplayComponent",
        text="DeviceID",
        width=60.0,
        x=20.0,
        y=20.0,
    )

    display_label = DisplayLabelModel(
        height=30.0,
        keys=[f"{device_id}.deviceId"],
        parent_component="DisplayComponent",
        width=540.0,
        x=80.0,
        y=20.0,
    )

    scene = SceneModel(
        height=470.0, width=630.0, children=[line, image, label, display_label]
    )

    scene.simple_name = f"Image_{device_id}_{path}"

    return scene
