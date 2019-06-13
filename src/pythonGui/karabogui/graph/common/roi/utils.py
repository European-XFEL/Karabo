import numpy as np

ROI_CENTER_HTML = (
    '<div style="text-align: center">'
    '<span style="color: #FFF; font-size: 8pt; font-weight: bold;">'
    'Cross Section</span><br>'
    '<span style="color: #FFF; font-size: 8pt;">'
    'Center: ({}, {})</span>'
    '</div>')

ROI_CENTER_SIZE_HTML = (
    '<div style="text-align: center">'
    '<span style="color: #FFF; font-size: 8pt; font-weight: bold;">'
    'Region of Interest</span><br>'
    '<span style="color: #FFF; font-size: 8pt;">'
    'Center: ({}, {})</span><br>'
    '<span style="color: #FFF; font-size: 8pt;">'
    'Size: ({}, {})</span>'
    '</div>')


class ImageRegion:
    Point = 0
    Line = 1
    Area = 2

    def __init__(self, region=None, region_type=None,
                 x_data=None, y_data=None):

        if region is None:
            region = np.empty((0, 0))
        if x_data is None:
            x_data = np.empty(0)
        if y_data is None:
            y_data = np.empty(0)

        self.region = region
        self.region_type = region_type
        self.axes = [x_data, y_data]

    def valid(self, axis=None):
        """Checks if region (all axis) is valid"""
        if self.region_type is ImageRegion.Area:
            # Region is a matrix, so we only check the shape
            return all(shape > 1 for shape in self.region.shape)

        if self.region_type is ImageRegion.Line:
            # Region is a list of two lists
            samples = (0, 1) if axis is None else (axis,)
            return all(self.region[ax].size > 1 for ax in samples)
