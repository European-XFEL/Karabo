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
from pathlib import Path

import numpy as np
from qtpy.QtGui import QValidator
from qtpy.QtWidgets import QSlider, QSpinBox

from karabogui.util import (
    InputValidator, SignalBlocker, _get_invalid_chars, convert_npy_to_csv,
    create_list_string, create_table_string, qtversion_compatible,
    version_compatible)


def test_assert_filename():
    def assert_filename(filename, *, invalid):
        invalid_chars = _get_invalid_chars(filename)
        assert set(list(invalid)) == set(invalid_chars)

    assert_filename("foo", invalid='')
    assert_filename("FoO/Bar-bAz_123", invalid='')
    assert_filename("foo bar", invalid=' ')
    assert_filename("foo!@#$%&*()+<>@:", invalid='!@#$%&*()+<>:')


def test_input_validator(gui_app):
    validator = InputValidator()
    valid, _, _ = validator.validate("1", 0)
    assert valid == QValidator.Acceptable

    valid, _, _ = validator.validate("w23-", 0)
    assert valid == QValidator.Acceptable

    valid, _, _ = validator.validate("w23-w+", 0)
    assert valid == QValidator.Invalid

    valid, _, _ = validator.validate("1+", 0)
    assert valid == QValidator.Invalid

    valid, _, _ = validator.validate("1#", 0)
    assert valid == QValidator.Invalid

    valid, _, _ = validator.validate("1|", 0)
    assert valid == QValidator.Invalid

    valid, _, _ = validator.validate("+a", 0)
    assert valid == QValidator.Invalid

    valid, _, _ = validator.validate("-a", 0)
    assert valid == QValidator.Acceptable

    # Macro case
    validator = InputValidator("macro")
    valid, _, _ = validator.validate("1", 0)
    assert valid == QValidator.Invalid

    valid, _, _ = validator.validate("-a", 0)
    assert valid == QValidator.Invalid

    valid, _, _ = validator.validate("+a", 0)
    assert valid == QValidator.Invalid

    valid, _, _ = validator.validate("w23", 0)
    assert valid == QValidator.Acceptable


def test_signal_blocker(gui_app):
    count = 0

    def slotReceived(*args, **kwargs):
        nonlocal count
        count += 1

    spinbox = QSpinBox()
    spinbox.valueChanged.connect(slotReceived)
    assert count == 0
    spinbox.setValue(100)
    assert count == 1

    # Update the spinbox, but block the signals
    with SignalBlocker(spinbox):
        for i in range(10):
            spinbox.setValue(i)
        assert spinbox.value() == i

    # Still only once
    assert count == 1
    spinbox.setValue(100)
    assert count == 2

    # Reset the counter and setup a slider
    count = 0

    slider = QSlider()
    slider.valueChanged.connect(slotReceived)
    slider.setRange(0, 100)
    slider.setValue(20)
    assert count == 1

    # Works, now block both
    with SignalBlocker(spinbox, slider):
        for i in range(10):
            spinbox.setValue(i)
            slider.setValue(i)
        assert spinbox.value() == i
        assert slider.value() == i
    assert count == 1

    # Other initialization
    count = 0
    blocker = SignalBlocker(spinbox)
    assert len(blocker.objects) == 1
    with blocker:
        spinbox.setValue(10000)
    assert count == 0


def test_version_compatible():
    """Test that we can verify versions"""
    assert version_compatible("2.14.0a2", 2, 14)
    assert version_compatible("2.14.0a2", 2, 13)
    assert version_compatible("2.14.0rc1", 2, 14)
    assert not version_compatible("2.14.0a2", 2, 15)
    assert version_compatible("asdeaeddevelopmode", 2, 14)
    assert version_compatible("asdeaeddevelopmode", 12, 33333)
    # minor smaller
    assert version_compatible("3.1.0a2", 2, 15)

    # Test garbage
    assert not version_compatible("2.&7as7.13", 2, 0)


def test_qversion_compatible():
    assert qtversion_compatible(5, 13)
    assert qtversion_compatible(5, 9)


def test_html_list_string():
    info = ["1", "2", 4, []]
    html = create_list_string(info)
    assert html == "<ul><li>1</li><li>2</li><li>4</li><li>[]</li></ul>"


def test_html_table_string():
    info = {"1": "a", 2: 3}
    html = create_table_string(info)
    assert html == "<table>" \
                   "<tr><td><b>1</b>:   </td><td>a</td></tr>" \
                   "<tr><td><b>2</b>:   </td><td>3</td></tr>" \
                   "</table>"


def test_npy2csv():
    """Test conversion of numpy files - npy and npz- to csv file."""
    # npy file.
    TEST_DATA_DIR = Path(__file__).resolve().parent / "data"
    input_file = TEST_DATA_DIR / "single_array.npy"
    assert input_file.exists()
    convert_npy_to_csv(input_file)
    output_file = TEST_DATA_DIR / "single_array.csv"

    x = np.arange(6.0000, dtype=float)
    y = np.arange(1.0, 12.0, step=2.0, dtype=float)
    expected = np.vstack((x, y))

    data_from_csv = np.genfromtxt(output_file, delimiter=',')
    np.testing.assert_array_equal(data_from_csv, expected)
    Path(output_file).unlink()

    # npz file.
    input_file = TEST_DATA_DIR / "multiple_arrays.npz"
    assert input_file.exists()
    convert_npy_to_csv(input_file)
    output_file = TEST_DATA_DIR / "multiple_arrays.csv"
    data_from_csv = np.genfromtxt(output_file, delimiter=',')

    x = np.arange(15.0, dtype=float)
    y1 = np.arange(10.0, 151.0, step=10.0, dtype=float)
    y2 = np.full(15, 10.0)
    y3 = np.arange(15.0, dtype=float)
    plot1 = np.vstack((x, y1))
    plot2 = np.vstack((x, y2))
    plot3 = np.vstack((x, y3))
    expected = np.vstack((plot1, plot2, plot3))

    np.testing.assert_array_equal(data_from_csv, expected)
    Path(output_file).unlink()
