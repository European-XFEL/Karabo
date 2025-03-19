# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from karabo.common.api import KARABO_SCHEMA_DISPLAY_TYPE
from karabo.common.display_types import (
    KARABO_SCHEMA_DISPLAY_TYPE_ALARM, KARABO_SCHEMA_DISPLAY_TYPE_BIN,
    KARABO_SCHEMA_DISPLAY_TYPE_HEX, KARABO_SCHEMA_DISPLAY_TYPE_STATE)
from karabo.common.scenemodel.api import (
    AlarmGraphModel, FilterTableElementModel, HistoricTextModel,
    StateGraphModel, TrendGraphModel, VectorGraphModel, WebCamGraphModel,
    get_alarm_graph_scene, get_image_scene, get_state_graph_scene,
    get_text_history_scene, get_trendline_scene, get_vector_scene)
from karabogui.binding.api import (
    BoolBinding, FloatBinding, ImageBinding, IntBinding, NDArrayBinding,
    NodeBinding, PipelineOutputBinding, StringBinding, VectorHashBinding,
    VectorNumberBinding, VectorStringBinding)


def _iter_binding(node, base=""):
    namespace = node.value
    base = base + "." if base else ""
    for name in namespace:
        subname = base + name
        subnode = getattr(namespace, name)
        if isinstance(subnode, NodeBinding):
            yield from _iter_binding(subnode, base=subname)
        yield subname, subnode


def get_generic_scene(proxy, include_images=True):
    """Return a generic scene for a proxy binding

    :param proxy: property proxy
    :param include_images: If image widget are included in the retrieval.
                           The default is `True`
    """
    binding = getattr(proxy, 'binding', None)
    if binding is None:
        return

    instance_id = proxy.root_proxy.device_id
    displayType = binding.attributes.get(KARABO_SCHEMA_DISPLAY_TYPE, '')
    if displayType == KARABO_SCHEMA_DISPLAY_TYPE_STATE:
        path = proxy.path
        return get_state_graph_scene(instance_id, path)
    elif displayType == KARABO_SCHEMA_DISPLAY_TYPE_ALARM:
        path = proxy.path
        return get_alarm_graph_scene(instance_id, path)

    if isinstance(binding, StringBinding):
        path = proxy.path
        return get_text_history_scene(instance_id, path)

    elif isinstance(binding, (BoolBinding, FloatBinding, IntBinding)):
        path = proxy.path
        return get_trendline_scene(instance_id, path)

    elif isinstance(binding, (NDArrayBinding, VectorNumberBinding)):
        path = proxy.path
        return get_vector_scene(instance_id, path)

    elif include_images and isinstance(binding, PipelineOutputBinding):
        for path, node in _iter_binding(binding, base=proxy.path):
            if isinstance(node, ImageBinding):
                return get_image_scene(instance_id, path)

    elif include_images and isinstance(binding, ImageBinding):
        return get_image_scene(instance_id, proxy.path)

    return None


def _get_plot_attributes(proxy):
    """Retrieve the basic plot model properties according to `proxy`"""
    binding = proxy.binding
    y_units = binding.unit_label if binding is not None else ""
    return {
        "y_label": proxy.path,
        "y_units": y_units,
        "width": 600,
        "height": 350,
        "x_grid": True,
        "y_grid": True,
        "keys": [proxy.key]
    }


def _get_widget_attributes(key):
    """Retrieve the basic image model properties according to `proxy`"""
    return {
        "width": 800,
        "height": 600,
        "keys": [key]
    }


def _get_table_attributes(key):
    """Retrieve the basic image model properties according to `proxy`"""
    return {
        "width": 800,
        "height": 600,
        "showFilterKeyColumn": True,
        "keys": [key]
    }


def get_property_proxy_model(proxy, include_images=True):
    """Return a generic model instance for a proxy binding

    :param proxy: property proxy
    :param include_images: If image widget are included in the retrieval.
                           The default is `True`
    """
    binding = getattr(proxy, "binding", None)
    if binding is None:
        return

    displayType = binding.attributes.get(
        KARABO_SCHEMA_DISPLAY_TYPE, "").split("|")[0]
    if displayType == KARABO_SCHEMA_DISPLAY_TYPE_STATE:
        return StateGraphModel(**_get_plot_attributes(proxy))
    elif displayType == KARABO_SCHEMA_DISPLAY_TYPE_ALARM:
        return AlarmGraphModel(**_get_plot_attributes(proxy))
    elif displayType in (
            KARABO_SCHEMA_DISPLAY_TYPE_BIN, KARABO_SCHEMA_DISPLAY_TYPE_HEX):
        return HistoricTextModel(**_get_widget_attributes(proxy.key))

    if isinstance(binding, (StringBinding, VectorStringBinding)):
        return HistoricTextModel(**_get_widget_attributes(proxy.key))

    elif isinstance(binding, VectorHashBinding):
        return FilterTableElementModel(**_get_table_attributes(proxy.key))

    elif isinstance(binding, (BoolBinding, FloatBinding, IntBinding)):
        return TrendGraphModel(**_get_plot_attributes(proxy))

    elif isinstance(binding, (NDArrayBinding, VectorNumberBinding)):
        return VectorGraphModel(**_get_plot_attributes(proxy))

    elif include_images and isinstance(binding, PipelineOutputBinding):
        for path, node in _iter_binding(binding, base=proxy.path):
            if isinstance(node, ImageBinding):
                path = f"{proxy.root_proxy.device_id}.{path}"
                return WebCamGraphModel(**_get_widget_attributes(path))

    elif include_images and isinstance(binding, ImageBinding):
        return WebCamGraphModel(**_get_widget_attributes(proxy.key))

    return None
