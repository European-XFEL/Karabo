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
# flake8: noqa
from .bases import (
    BaseDisplayEditableWidget, BaseEditWidget, BaseLayoutData, BaseLayoutModel,
    BaseSceneObjectData, BaseShapeObjectData, BaseWidgetObjectData,
    XMLElementModel)
from .const import (
    DISPLAY_COMPONENT, EDITABLE_COMPONENT, NS_KARABO, NS_SVG,
    SCENE_DEFAULT_DPI, SCENE_FILE_VERSION, SCENE_FONT_FAMILY, SCENE_FONT_SIZE,
    SCENE_FONT_WEIGHT, SCENE_MAC_DPI, SCENE_MIN_HEIGHT, SCENE_MIN_WIDTH,
    WIDGET_ELEMENT_TAG, SceneTargetWindow)
from .exceptions import SceneReaderException, SceneWriterException
from .generic_scenes import (
    get_alarm_graph_scene, get_image_scene, get_state_graph_scene,
    get_text_history_scene, get_trendline_scene, get_vector_scene)
from .layouts import (
    BoxLayoutModel, FixedLayoutChildData, FixedLayoutModel,
    GridLayoutChildData, GridLayoutModel)
from .model import (
    SceneModel, UnknownWidgetDataModel, UnknownXMLDataModel, set_scene_reader)
from .modelio import read_scene, write_scene, write_single_model
from .registry import register_scene_reader, register_scene_writer
from .shapes import ArrowPolygonModel, LineModel, PathModel, RectangleModel
from .widgets.complex import (
    ColorBoolModel, DisplayCommandModel, DisplayIconCommandModel,
    DisplayProgressBarModel, DisplayStateColorModel, DoubleLineEditModel,
    ErrorBoolModel, EvaluatorModel, FilterTableElementModel, FloatSpinBoxModel,
    MonitorModel, SingleBitModel, TableElementModel)
# deprecated widgets
# Graph widgets
from .widgets.graph_image import (
    DetectorGraphModel, ImageGraphModel, VectorRollGraphModel,
    WebCamGraphModel)
from .widgets.graph_plots import (
    AlarmGraphModel, BasePlotModel, MultiCurveGraphModel, NDArrayGraphModel,
    ScatterGraphModel, StateGraphModel, TrendGraphModel, VectorBarGraphModel,
    VectorFillGraphModel, VectorGraphModel, VectorHistGraphModel,
    VectorScatterGraphModel, VectorXYGraphModel)
from .widgets.graph_utils import (
    BaseROIData, CrossROIData, PlotType, RectROIData, build_graph_config,
    build_model_config, read_axes_set, read_base_karabo_image_model,
    read_basic_label, read_range_set, read_roi_info, read_transforms,
    restore_graph_config, write_axes_set, write_base_karabo_image_model,
    write_basic_label, write_range_set, write_roi_info, write_transforms)
from .widgets.icon import (
    BaseIconsModel, DigitIconsModel, DisplayIconsetModel, IconData,
    SelectionIconsModel, TextIconsModel)
from .widgets.links import DeviceSceneLinkModel, SceneLinkModel, WebLinkModel
from .widgets.plot import SparklineModel
from .widgets.simple import (
    BaseLabelModel, CheckBoxModel, DisplayAlarmFloatModel,
    DisplayAlarmIntegerModel, DisplayFloatModel, DisplayLabelModel,
    DisplayListModel, DisplayTextLogModel, DisplayTimeModel,
    EditableChoiceElementModel, EditableComboBoxModel,
    EditableListElementModel, EditableListModel, EditableRegexListModel,
    EditableRegexModel, EditableSpinBoxModel, GlobalAlarmModel,
    HexadecimalModel, HistoricTextModel, InstanceStatusModel, IntLineEditModel,
    LabelModel, LampModel, LineEditModel, PopupButtonModel, StickerModel,
    TickSliderModel, WidgetNodeModel)
from .widgets.statefulicon import StatefulIconWidgetModel
from .widgets.tools import (
    ImageRendererModel, create_base64image, extract_base64image)
