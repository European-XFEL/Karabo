# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import uuid

from karabo.common.project.api import MacroModel


def test_instance_id_property():
    UUID = str(uuid.uuid4())
    macro = MacroModel(simple_name="foo", uuid=UUID)

    assert macro.instance_id == f"Macro-foo-{UUID}"
    macro.simple_name = "bar"
    assert macro.instance_id == f"Macro-bar-{UUID}"
