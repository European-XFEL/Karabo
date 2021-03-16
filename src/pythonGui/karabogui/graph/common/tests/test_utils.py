import numpy as np

from ..utils import clip_array


def test_clip_array():
    """Test the clipping of arrays for the installed numpy version"""
    arr = np.array([0, 20, 7, 37, -1, 2, 3, 4, 5, 1, 2, 3, 4, 5])
    new_arr = clip_array(arr, 0, 10)
    assert new_arr[1] == 10
    assert new_arr[3] == 10
    assert new_arr[4] == 0
    assert len(new_arr) == len(arr)
