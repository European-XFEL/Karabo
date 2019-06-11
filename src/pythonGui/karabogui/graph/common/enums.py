from enum import Enum, IntEnum


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
