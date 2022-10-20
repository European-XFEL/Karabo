# flake8: noqa
from .selection_model import SceneSelectionModel
from .shapes import LineShape, RectangleShape
from .view import SceneView
from .widget.api import (
    ControllerContainer, ImageRendererWidget, LabelWidget, SceneLinkWidget,
    StickerWidget, UnknownSvgWidget, UnknownWidget, WebLinkWidget)
from .widget.utils import get_proxy
