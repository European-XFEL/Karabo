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
import numpy as np
from pyqtgraph import mkColor
from pyqtgraph.exporters import ImageExporter as PgImageExporter

from karabo.native import Timestamp
from karabogui import util
from karabogui.singletons.api import get_config


class PlotDataExporter:
    def __init__(self, data_items):
        if len(data_items) == 1:
            x_data, y_data = data_items[0].getData()
            self._exporter = ArrayExporter(self._stacked_xy(x_data, y_data))
        else:
            saved = {curve.name(): self._stacked_xy(*curve.getData())
                     for curve in data_items}
            self._exporter = MultiArrayExporter(saved)

    def export(self):
        self._exporter.export()

    def _stacked_xy(self, x_data, y_data):
        if x_data is None and y_data is None:
            x_data, y_data = [], []

        # Adjust size if not same, usually happens with histogram plots,
        # wherein the last element of the x-axis can be removed
        x_size, y_size = len(x_data), len(y_data)
        if x_size != y_size:
            if x_size > y_size:
                x_data = x_data[:y_size]
            else:
                y_data = y_data[:x_size]

        return np.vstack((x_data, y_data))


class ArrayExporter:
    """Exporter for single numpy arrays. This is typically used for images and
       plots with single curves.

       For the case of the latter, the x- and y-components are aggregated into
       one array:
          x = np.array([1,2,3])
          y = np.array([4,5,6])
          data = [x, y]

       Data is saved as:
          np.save("foo.npy", data)

       And can be then accessed as:
          saved_data = np.load("foo.npy")
          saved_data
             >> np.array([[1,2,3],[4,5,6]])"""

    def __init__(self, data):
        if not isinstance(data, np.ndarray):
            raise TypeError(f"Data must be a Numpy array, not {type(data)}.")
        self._data = data

    def export(self):
        name = f"{Timestamp().toLocal()}_data.npy"
        filename = util.getSaveFileName(caption="Export Data",
                                        filter="Numpy File (*.npy)",
                                        suffix="npy",
                                        directory=get_config()["data_dir"],
                                        selectFile=name)

        if not filename:
            return

        if not filename.endswith(".npy"):
            filename = f"{filename}.npy"

        np.save(filename, self._data)


class MultiArrayExporter:
    """Exporter for multiple numpy arrays. This is typically used for
       multicurve plots. This utilizes proxy names as keys:

       data_1 = np.array([[1,2,3],[4,5,6]])
       data_2 = np.array([[7,8],[9,10]])


       Data is saved as:
          np.savez("foo.py", proxy_1=data_1, proxy_2=data_2)

       And can be then accessed as:
          saved_data = np.load("foo.npz")

          saved_data["proxy_1"]
            >> np.array([[1,2,3],[4,5,6]])

          saved_data["proxy_2"]
            >> np.array([[7,8],[9,10]])

       The x- and y-components are aggregated into one array, similar with
       `ArrayExporter`.
       """

    def __init__(self, zipped):
        for data in zipped.values():
            if not isinstance(data, np.ndarray):
                raise TypeError(f"Data must be a Numpy array, "
                                f"not {type(data)}.")
        self._zipped = zipped

    def export(self):
        name = f"{Timestamp().toLocal()}_data.npz"
        filename = util.getSaveFileName(caption="Export Data",
                                        filter="Numpy Zipped File (*.npz)",
                                        suffix="npz",
                                        directory=get_config()["data_dir"],
                                        selectFile=name)

        if not filename:
            return

        if not filename.endswith(".npz"):
            filename = f"{filename}.npz"

        np.savez(filename, **self._zipped)


class ImageExporter:
    def __init__(self, item):
        self._exporter = PgImageExporter(item)
        params = self._exporter.parameters()
        params["antialias"] = False
        params["background"] = mkColor("w")

    def export(self):
        """Exports the plot to an image"""
        # First we export the contents to a QImage
        qimage = self._exporter.export(toBytes=True)
        name = f"{Timestamp().toLocal()}_image.png"
        filename = util.getSaveFileName(caption="Save Snapshot",
                                        filter="PNG (*.png)",
                                        suffix="png",
                                        directory=get_config()["data_dir"],
                                        selectFile=name)

        if not filename:
            return

        if not filename.endswith(".png"):
            filename = f"{filename}.png"

        qimage.save(filename)
