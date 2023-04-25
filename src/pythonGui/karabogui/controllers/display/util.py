# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from pathlib import Path


def get_ui_file(ui_name):
    """Retrieve the appropriate `.ui` file from the ui folder"""
    assert ui_name.endswith(".ui"), f"{ui_name} is not a `.ui` file"

    path = Path(__file__).parent / "ui" / ui_name
    return path
