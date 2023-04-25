# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import os.path as op


def get_dialog_ui(ui_name):
    """Retrieve the appropriate `.ui` file from the dialog ui folder
    """
    assert ui_name.endswith(".ui"), f"{ui_name} is not a `.ui` file"

    return op.join(op.abspath(op.dirname(__file__)), "ui", ui_name)
