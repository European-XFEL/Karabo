# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from enum import Enum, IntEnum


class Axes(IntEnum):
    Y = 1
    X = 0
    Z = 2


class AxisType(Enum):
    AuxPlot = "AuxPlots"
    Alarm = "Alarm"
    Classic = "Classic"
    State = "State"
    Time = "Time"


class MouseTool(IntEnum):
    # Default
    Pointer = 0
    Zoom = 1
    Move = 2

    # Optional
    Picker = 3


# Backward compatibility
MouseMode = MouseTool


class ExportTool(Enum):
    Image = "All items"
    Data = "npy"


class AuxPlots(IntEnum):
    NoPlot = 0
    ProfilePlot = 1
    Histogram = 2


class ROITool(IntEnum):
    NoROI = 0
    Rect = 1
    Crosshair = 2

    DrawRect = 11
    DrawCrosshair = 12


class AspectRatio(IntEnum):
    NoAspectRatio = 0
    PixelDependent = 1
    ScaleDependent = 2
