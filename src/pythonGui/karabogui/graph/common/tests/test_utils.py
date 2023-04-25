# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from ..utils import float_to_string


def test_float_string():
    assert float_to_string(3.002) == "3"
    assert float_to_string(3.002, precision=3) == "3.002"
    assert float_to_string(3, precision=3) == "3"
    assert float_to_string(1e9, precision=3) == "1000000000"
    assert float_to_string(-3.0023344) == "-3"
    assert float_to_string(-3.0023344, precision=3) == "-3.002"
    assert float_to_string(-3.0023344, precision=4) == "-3.0023"
