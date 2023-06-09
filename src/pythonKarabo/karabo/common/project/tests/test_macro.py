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
import uuid

from karabo.common.project.api import MacroModel


def test_instance_id_property():
    UUID = str(uuid.uuid4())
    macro = MacroModel(simple_name="foo", uuid=UUID)

    assert macro.instance_id == f"Macro-foo-{UUID}"
    macro.simple_name = "bar"
    assert macro.instance_id == f"Macro-bar-{UUID}"
