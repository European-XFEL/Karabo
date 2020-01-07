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
                 x_slice=None, y_slice=None):

        if region is None:
            region = np.empty((0, 0))
        if x_slice is None:
            x_slice = np.empty(0)
        if y_slice is None:
            y_slice = np.empty(0)

        self.region = region
        self.region_type = region_type
        self.slices = [x_slice, y_slice]

    def valid(self, axis=None):
        """Checks if region (all axis) is valid"""
        if self.region_type is ImageRegion.Area:
            # Region is a matrix, so we only check the shape
            return all(shape > 1 for shape in self.region.shape)

        if self.region_type is ImageRegion.Line:
            # Region is a list of two lists
            samples = (0, 1) if axis is None else (axis,)
            return all(self.region[ax].size > 1 for ax in samples)

        return False

    def flatten(self):
        if self.region_type is ImageRegion.Area:
            # region is a numpy array
            return self.region.flatten()

        if self.region_type is ImageRegion.Line:
            # region is a list of 2 lists (x and y most probably have different
            # shapes)
            return np.hstack(self.region)
