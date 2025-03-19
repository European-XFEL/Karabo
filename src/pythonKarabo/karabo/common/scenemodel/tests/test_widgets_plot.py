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
from io import StringIO

from ..api import SparklineModel, read_scene
from .utils import (
    assert_base_traits, base_widget_traits, single_model_round_trip)

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


def test_sparkline_basics():
    traits = base_widget_traits()
    traits["time_base"] = 42
    traits["show_value"] = True
    traits["show_format"] = "whatever"
    model = SparklineModel(**traits)
    read_model = single_model_round_trip(model)
    assert_base_traits(read_model)
    assert model.time_base == 42
    assert model.show_value
    assert model.show_format == "whatever"


def test_old_sparkline_data():
    default = SparklineModel()
    with StringIO(OLD_SPARKY) as fp:
        scene = read_scene(fp)

    model = scene.children[0]
    assert isinstance(model, SparklineModel)
    assert model.keys == ["thePast.sparkProp"]
    assert model.time_base == default.time_base
    assert model.show_value == default.show_value
    assert model.show_format == default.show_format
