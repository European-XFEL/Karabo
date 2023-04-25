# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
def status_icon_path(instance):
    """Return the status icon method path for an instance"""
    path = (f"karabogui.project.controller.{instance}."
            f"get_project_{instance}_status_icon")
    return path
