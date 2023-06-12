#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 16, 2016
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
#############################################################################

from pathlib import Path

from qtpy.QtCore import QByteArray
from qtpy.QtSvg import QSvgRenderer

from karabo.common.scenemodel.api import (
    LabelModel, LineModel, RectangleModel, UnknownWidgetDataModel)
from karabogui import icons
from karabogui.sceneview.api import (
    LabelWidget, LineShape, RectangleShape, SceneSelectionModel,
    UnknownSvgWidget, UnknownWidget)


def test_selection_model_types(gui_app):
    selection_model = SceneSelectionModel()
    assert not selection_model.has_selection()
    endpoint_x, endpoint_y = (100, 100)

    line_model = LineModel(x1=0, y1=0, x2=endpoint_x, y2=endpoint_y)
    line_shape = LineShape(model=line_model)
    selection_model.select_object(line_shape)
    assert selection_model.has_selection()

    selection_model.clear_selection()
    assert not selection_model.has_selection()

    rect_model = RectangleModel(x=endpoint_x, y=endpoint_y,
                                width=100, height=100)
    rect_shape = RectangleShape(model=rect_model)
    selection_model.select_object(rect_shape)
    assert selection_model.has_selection()

    selection_model.clear_selection()
    assert not selection_model.has_selection()

    selection_model.select_object(line_shape)
    selection_model.select_object(rect_shape)

    for obj in selection_model:
        assert obj in (line_shape, rect_shape)

    exp_rect = line_shape.geometry().united(rect_shape.geometry())
    sel_rect = selection_model.get_selection_bounds()
    assert exp_rect == sel_rect

    selection_model.deselect_object(line_shape)
    assert selection_model.has_selection()

    selection_model.deselect_object(rect_shape)
    assert not selection_model.has_selection()


def test_selection_model_unknown(gui_app):
    selection_model = SceneSelectionModel()
    assert not selection_model.has_selection()

    label_model = LabelModel()
    label_widget = LabelWidget(label_model)

    unknown_model = UnknownWidgetDataModel(
        klass="Notthere",
        data="",
        attributes={})
    unknown_widget = UnknownWidget(unknown_model)

    selection_model.select_object(label_widget)
    selection_model.select_object(unknown_widget)
    assert selection_model.has_selection()
    assert len(selection_model) == 2

    path = Path(icons.__file__).parent / "about.svg"
    with open(path, "rb") as fp:
        b = fp.read()

    ar = QByteArray.fromRawData(b)
    renderer = QSvgRenderer(ar)
    assert renderer.isValid()
    svg_widget = UnknownSvgWidget(renderer=renderer)

    # Try to select an unknown svg widget does nothing
    selection_model.select_object(svg_widget)
    assert selection_model.has_selection()
    assert len(selection_model) == 2
