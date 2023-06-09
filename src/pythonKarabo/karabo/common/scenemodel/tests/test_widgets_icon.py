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
from pytest import raises as assert_raises

from .. import api
from .utils import (
    assert_base_traits, base_widget_traits, single_model_from_data,
    single_model_round_trip)

VERSION_1_DIGIT_ICONS_SVG = """
<svg
    xmlns:krb="http://karabo.eu/scene"
    xmlns:svg="http://www.w3.org/2000/svg"
    height="768"
    width="1024"
    krb:version="1">
        <svg:rect
            height="100"
            width="100"
            x="0"
            y="0"
            krb:class="DisplayComponent"
            krb:keys="device_id.prop"
            krb:widget="DigitIcons">
                <krb:value image="blah.svg" equal="false">14</krb:value>
        </svg:rect>
</svg>
"""
VERSION_1_DISPLAY_ICONSET_SVG = """
<svg
    xmlns:krb="http://karabo.eu/scene"
    xmlns:svg="http://www.w3.org/2000/svg"
    height="768"
    width="1024"
    krb:version="1">
        <svg:rect
            height="100"
            width="100"
            x="0"
            y="0"
            krb:class="DisplayComponent"
            krb:keys="device_id.prop"
            krb:url="blah.svg"
            krb:widget="DisplayIconset" />
</svg>
"""


def _check_icon_widget(klass):
    traits = base_widget_traits()
    icon = api.IconData(data=b"karabo")
    if klass is api.DigitIconsModel:
        icon.equal = True
        icon.value = "14"
    traits["values"] = [icon]
    model = klass(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert len(read_model.values) == 1
    assert read_model.values[0].data == b"karabo"
    if klass is api.DigitIconsModel:
        assert read_model.values[0].equal is True
        assert read_model.values[0].value == "14"


def test_display_iconset_widget():
    traits = base_widget_traits()
    traits["data"] = b"karabo"
    model = api.DisplayIconsetModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert read_model.image == 'data:image/png;base64,a2FyYWJv'


def test_icon_widgets():
    model_classes = (
        api.DigitIconsModel,
        api.SelectionIconsModel,
        api.TextIconsModel,
    )
    for klass in model_classes:
        _check_icon_widget(klass)


def test_display_iconset_widget_version_1():
    read_model = single_model_from_data(VERSION_1_DISPLAY_ICONSET_SVG)
    assert_base_traits(read_model)
    assert read_model.image == 'data:image/svg;base64,'


def test_icon_widget_version_1():
    read_model = single_model_from_data(VERSION_1_DIGIT_ICONS_SVG)
    assert_base_traits(read_model)
    assert len(read_model.values) == 1
    assert read_model.values[0].image == "blah.svg"
    assert isinstance(read_model, api.DigitIconsModel)


def test_write_exceptions():
    traits = base_widget_traits()
    traits["values"] = [api.IconData()]
    model = api.TextIconsModel(**traits)
    assert_raises(api.SceneWriterException, single_model_round_trip, model)

    traits = base_widget_traits()
    model = api.DisplayIconsetModel(**traits)
    assert_raises(api.SceneWriterException, single_model_round_trip, model)
