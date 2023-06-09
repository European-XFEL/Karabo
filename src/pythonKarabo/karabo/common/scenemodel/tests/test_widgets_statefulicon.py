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
from ..api import StatefulIconWidgetModel
from .utils import (
    assert_base_traits, base_widget_traits, single_model_round_trip)

STATEFUL_ICON_WIDGETS = ["foo", "bar", "foobar"]


def test_statefulicon_widget():
    model = StatefulIconWidgetModel()
    for name in STATEFUL_ICON_WIDGETS:
        traits = base_widget_traits()
        traits["icon_name"] = name
        model = StatefulIconWidgetModel(**traits)
        read_model = single_model_round_trip(model)
        assert_base_traits(read_model)
        assert read_model.icon_name == name
