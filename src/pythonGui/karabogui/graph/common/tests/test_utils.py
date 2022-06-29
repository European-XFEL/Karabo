import numpy as np

from ..utils import clip_array, float_to_string


def test_clip_array():
    """Test the clipping of arrays for the installed numpy version"""
    arr = np.array([0, 20, 7, 37, -1, 2, 3, 4, 5, 1, 2, 3, 4, 5])
    new_arr = clip_array(arr, 0, 10)
    assert new_arr[1] == 10
    assert new_arr[3] == 10
    assert new_arr[4] == 0
    assert len(new_arr) == len(arr)


def test_float_string():
    assert float_to_string(3.002) == "3"
    assert float_to_string(3.002, precision=3) == "3.002"
    assert float_to_string(3, precision=3) == "3"
    assert float_to_string(1e9, precision=3) == "1000000000"
    assert float_to_string(-3.0023344) == "-3"
    assert float_to_string(-3.0023344, precision=3) == "-3.002"
    assert float_to_string(-3.0023344, precision=4) == "-3.0023"
