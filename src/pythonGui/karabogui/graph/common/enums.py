from enum import Enum, IntEnum


class Axes(Enum):
    Y = 1
    X = 0
    Z = 2


class AxisType(Enum):
    State = "State"
    Time = "Time"
    AuxPlot = "AuxPlots"
    Classic = "Classic"


class MouseMode(IntEnum):
    # Default
    Pointer = 0
    Zoom = 1
    Move = 2

    # Optional
    Picker = 3


class ExportTool(Enum):
    Image = "All items"
    Data = "npy"


class AuxPlots(IntEnum):
    NoPlot = 0
    ProfilePlot = 1


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
