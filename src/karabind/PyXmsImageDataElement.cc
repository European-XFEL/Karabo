/*
 *
 * Author: CTRL DEV
 * Created on April 16, 2023, 9:53 AM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <karabo/xms/ImageData.hh>

#include "FromNumpy.hh"
#include "ToNumpy.hh"
#include "Wrapper.hh"
#include "karabo/data/types/Dims.hh"
#include "karabo/data/types/FromLiteral.hh"
#include "karabo/data/types/NDArray.hh"


namespace py = pybind11;
using namespace karabo::data;
using namespace karabo::xms;
using namespace karabind;


void exportPyXmsImageDataElement(py::module_& m) {
    {
        py::enum_<Encoding>(m, "Encoding")
              .value("UNDEFINED", Encoding::UNDEFINED)
              .value("GRAY", Encoding::GRAY)
              .value("RGB", Encoding::RGB)
              .value("RGBA", Encoding::RGBA)
              .value("BGR", Encoding::BGR)
              .value("BGRA", Encoding::BGRA)
              .value("CMYK", Encoding::CMYK)
              .value("YUV", Encoding::YUV)
              .value("BAYER", Encoding::BAYER)
              .value("JPEG", Encoding::JPEG)
              .value("PNG", Encoding::PNG)
              .value("BMP", Encoding::BMP)
              .value("TIFF", Encoding::TIFF)
              .value("YUV444", Encoding::YUV444)
              .value("YUV422_YUYV", Encoding::YUV422_YUYV)
              .value("YUV422_UYVY", Encoding::YUV422_UYVY)
              .value("BAYER_RG", Encoding::BAYER_RG)
              .value("BAYER_BG", Encoding::BAYER_BG)
              .value("BAYER_GR", Encoding::BAYER_GR)
              .value("BAYER_GB", Encoding::BAYER_GB)
              .export_values();
    }

    {
        py::enum_<Rotation>(m, "Rotation")
              .value("UNDEFINED", Rotation::UNDEFINED)
              .value("ROT_0", Rotation::ROT_0)
              .value("ROT_90", Rotation::ROT_90)
              .value("ROT_180", Rotation::ROT_180)
              .value("ROT_270", Rotation::ROT_270)
              .export_values();
    }

    {
        py::enum_<DimensionType>(m, "DimensionType")
              .value("UNDEFINED", DimensionType::UNDEFINED)
              .value("STACK", DimensionType::STACK)
              .value("DATA", DimensionType::DATA);
    }

    {
        py::class_<ImageData, std::shared_ptr<ImageData>> img(m, "ImageData");

        img.def(py::init<>());

        // Custom constructors: https://pybind11.readthedocs.io/en/stable/advanced/classes.html#custom-constructors
        img.def(py::init([](const py::array& arr, const Dims& dimensions, const Encoding encoding,
                            const int bitsPerPixel) {
                    return std::make_shared<ImageData>(wrapper::castPyArrayToND(arr), dimensions, encoding,
                                                       bitsPerPixel);
                }),
                py::arg("array"), py::arg("dims") = Dims(), py::arg("encoding") = Encoding::UNDEFINED,
                py::arg("bitsPerPixel") = 0);

        // Dimensions are deduced from ndarray
        img.def(py::init([](const py::array& arr, const Encoding encoding, const int bitsPerPixel) {
                    return std::make_shared<ImageData>(wrapper::castPyArrayToND(arr), encoding, bitsPerPixel);
                }),
                py::arg("array"), py::arg("encoding") = Encoding::UNDEFINED, py::arg("bitsPerPixel") = 0);

        img.def("getData", [](const ImageData& self) { return wrapper::castNDArrayToPy(self.getData()); });

        img.def(
              "setData", [](ImageData& self, const py::array& arr) { self.setData(wrapper::castPyArrayToND(arr)); },
              py::arg("arr"));

        img.def("getDataCopy", [](const ImageData& self) { return wrapper::copyNDArrayToPy(self.getData()); });

        img.def(
              "setDataCopy", [](ImageData& self, const py::array& arr) { self.setData(wrapper::copyPyArrayToND(arr)); },
              py::arg("arr"));

        img.def("getType", [](const ImageData& self) { return py::cast(static_cast<int>(self.getDataType())); });

        img.def(
              "setType",
              [](ImageData& self, const py::object& otype) {
                  Types::ReferenceType type = wrapper::pyObjectToCppType(otype);
                  self.setDataType(type);
              },
              py::arg("dtype"));


        img.def("getDimensions", [](const ImageData& self) -> py::tuple {
            Dims dims = self.getDimensions();
            std::vector<unsigned long long> v = dims.toVector();
            return py::cast(v);
        });

        img.def(
              "setDimensions",
              [](ImageData& self, const py::object& o) {
                  using namespace karabo::data;
                  if (py::isinstance<Dims>(o)) {
                      self.setDimensions(o.cast<Dims>());
                  } else if (py::isinstance<py::sequence>(o)) {
                      const auto& v = o.cast<std::vector<unsigned long long>>();
                      // std::vector<unsigned long long> v;
                      // for (unsigned long long item : o) v.push_back(item);
                      Dims dims(v);
                      self.setDimensions(dims);
                  } else {
                      throw KARABO_PYTHON_EXCEPTION(std::string("Unsupported argument type '") +
                                                    o.ptr()->ob_type->tp_name + "'");
                  }
              },
              py::arg("dims"));

        img.def("getDimensionTypes", [](const ImageData& self) -> py::tuple {
            std::vector<DimensionType> v = self.getDimensionTypes();
            return py::cast(v);
        });

        img.def(
              "setDimensionTypes",
              [](ImageData& self, const py::sequence& o) {
                  self.setDimensionTypes(o.cast<std::vector<DimensionType>>());
                  //   py::ssize_t size = py::len(o);
                  //   std::vector<int> dimTypes(size);
                  //   for (int i = 0; i < size; i++) dimTypes[i] = o[i].cast<int>();
                  //   self.setDimensionTypes(dimTypes);
              },
              py::arg("listOfDimTypes"));

        img.def("getROIOffsets", [](const ImageData& self) -> py::tuple {
            using namespace karabo::data;
            Dims offsets = self.getROIOffsets();
            std::vector<unsigned long long> v = offsets.toVector();
            return py::cast(v);
        });

        img.def(
              "setROIOffsets",
              [](ImageData& self, const py::object& o) {
                  using namespace karabo::data;
                  if (py::isinstance<Dims>(o)) {
                      self.setROIOffsets(o.cast<Dims>());
                  } else if (py::isinstance<py::sequence>(o)) {
                      const auto& v = o.cast<std::vector<unsigned long long>>();
                      self.setROIOffsets(v);
                  } else {
                      throw KARABO_PYTHON_EXCEPTION(std::string("Unsupported argument type '") +
                                                    o.ptr()->ob_type->tp_name + "'");
                  }
              },
              py::arg("offsets"));

        img.def("getBinning", [](const ImageData& self) -> py::tuple {
            using namespace karabo::data;
            Dims binning = self.getBinning();
            std::vector<unsigned long long> v = binning.toVector();
            return py::cast(v);
        });

        img.def(
              "setBinning",
              [](ImageData& self, const py::object& o) {
                  using namespace karabo::data;
                  if (py::isinstance<Dims>(o)) {
                      self.setBinning(o.cast<Dims>());
                  } else if (py::isinstance<py::sequence>(o)) {
                      const auto& binning = o.cast<std::vector<unsigned long long>>();
                      self.setBinning(binning);
                  } else {
                      throw KARABO_PYTHON_EXCEPTION(std::string("Unsupported argument type '") +
                                                    o.ptr()->ob_type->tp_name + "'");
                  }
              },
              py::arg("binning"));

        img.def("getRotation", &ImageData::getRotation);

        img.def("setRotation", &ImageData::setRotation, py::arg("rotation"));

        img.def("getFlipX", &ImageData::getFlipX);

        img.def("setFlipX", &ImageData::setFlipX, py::arg("flipX"));

        img.def("getFlipY", &ImageData::getFlipY);

        img.def("setFlipY", &ImageData::setFlipY, py::arg("flipY"));

        img.def("getBitsPerPixel", &ImageData::getBitsPerPixel);

        img.def("setBitsPerPixel", &ImageData::setBitsPerPixel, py::arg("bitsPerPixel"));

        img.def("getEncoding", [](const ImageData& self) { return py::cast(Encoding(self.getEncoding())); });

        img.def("setEncoding", &ImageData::setEncoding, py::arg("encoding"));

        img.def("isIndexable", &ImageData::isIndexable);

        img.def("getDimensionScales", &ImageData::getDimensionScales);

        img.def("setDimensionScales", &ImageData::setDimensionScales, py::arg("scales"));
    }
    {
        py::class_<ImageDataElement> el(m, "IMAGEDATA_ELEMENT");

        el.def(py::init<Schema&>(), py::arg("expected"));

        el.def("key", &ImageDataElement::key, py::arg("key"), py::return_value_policy::reference_internal);

        el.def("displayedName", &ImageDataElement::displayedName, py::arg("name"),
               py::return_value_policy::reference_internal);

        el.def("description", &ImageDataElement::description, py::arg("desc"),
               py::return_value_policy::reference_internal);

        // el.def("setDefaultValue", &karathon::ImageDataElementWrap().setDefaultValue, py::arg("subKey"),
        //        py::arg("defaultValue"), py::return_value_policy::reference_internal);

        el.def("observerAccess", &ImageDataElement::observerAccess, py::return_value_policy::reference_internal);

        el.def("operatorAccess", &ImageDataElement::operatorAccess, py::return_value_policy::reference_internal);

        el.def("expertAccess", &ImageDataElement::expertAccess, py::return_value_policy::reference_internal);

        el.def("skipValidation", &ImageDataElement::skipValidation, py::return_value_policy::reference_internal);

        el.def("commit", &ImageDataElement::commit, py::return_value_policy::reference_internal);

        el.def("setDimensionScales", &ImageDataElement::setDimensionScales, py::arg("scales"));

        el.def(
              "setDimensions",
              [](ImageDataElement& self, const py::object& o) {
                  if (py::isinstance<py::str>(o)) {
                      // If image dimensions were given as string
                      return self.setDimensions(o.cast<std::string>());
                  } else if (py::isinstance<py::list>(o)) {
                      // If image dimensions were given as list
                      return self.setDimensions(o.cast<std::vector<unsigned long long>>());
                  } else {
                      throw KARABO_PYTHON_EXCEPTION(
                            "Python type of setDimensions() of ImageDataElement must be a list or a string");
                  }
              },
              py::arg("dims"), py::return_value_policy::reference_internal,
              "h.setDimensions(dims) set the shape of an image in the schema. This is required by the DAQ,"
              "otherwise silent data-loss or segfaults can occur. The shape can be a list or string with 2 or "
              "3 "
              "dimensions (for color images).\nExample:\n\t"
              "IMAGEDATA_ELEMENT(s)\n\t\t.key('data.image')\n\t\t.setDimensions('480,640,3')\n\t\t.commit()"
              "\n\t"
              "IMAGEDATA_ELEMENT(s)\n\t\t.key('data.image')\n\t\t.setDimensions([480,640])\n\t\t.commit()");

        el.def(
              "setType",
              [](ImageDataElement& self, const py::object& otype) {
                  return self.setType(wrapper::pyObjectToCppType(otype));
              },
              py::arg("type"), py::return_value_policy::reference_internal,
              "h.setType(type) set the datatype of an image in the schema. This is required by the DAQ,"
              "otherwise silent data-loss or segfaults can occur. The type can be a member of the 'Types' "
              "class or a string (all capitals).\nExample:\n\t"
              "IMAGEDATA_ELEMENT(s)\n\t\t.key('data.image')\n\t\t.setType('UINT16')\n\t\t.commit()\n\t"
              "IMAGEDATA_ELEMENT(s)\n\t\t.key('data.image')\n\t\t.setType(Types.UINT16)\n\t\t.commit()");

        el.def(
              "setEncoding",
              [](ImageDataElement& self, const py::object& o) {
                  using namespace karabo::xms;
                  Encoding encType = Encoding::UNDEFINED;
                  // If image encoding type is given as string
                  if (py::isinstance<py::str>(o)) {
                      // Create look-up table to translate string -
                      //      keys to Encoding
                      std::map<std::string, Encoding> encLuT;
                      encLuT["UNDEFINED"] = Encoding::UNDEFINED;
                      encLuT["GRAY"] = Encoding::GRAY;
                      encLuT["RGB"] = Encoding::RGB;
                      encLuT["RGBA"] = Encoding::RGBA;
                      encLuT["BGR"] = Encoding::BGR;
                      encLuT["BGRA"] = Encoding::BGRA;
                      encLuT["CMYK"] = Encoding::CMYK;
                      encLuT["YUV"] = Encoding::YUV;
                      encLuT["YUV444"] = Encoding::YUV444;
                      encLuT["YUV422_YUYV"] = Encoding::YUV422_YUYV;
                      encLuT["YUV422_UYVY"] = Encoding::YUV422_UYVY;
                      encLuT["BAYER"] = Encoding::BAYER;
                      encLuT["BAYER_RG"] = Encoding::BAYER_RG;
                      encLuT["BAYER_BG"] = Encoding::BAYER_BG;
                      encLuT["BAYER_GR"] = Encoding::BAYER_GR;
                      encLuT["BAYER_GB"] = Encoding::BAYER_GB;
                      encLuT["JPEG"] = Encoding::JPEG;
                      encLuT["PNG"] = Encoding::PNG;
                      encLuT["BMP"] = Encoding::BMP;
                      encLuT["TIFF"] = Encoding::TIFF;
                      // Look up the supplied key in the LUT
                      encType = encLuT[o.cast<std::string>()];
                      // If data type was given as integer
                  } else if (py::isinstance<Encoding>(o)) {
                      encType = o.cast<Encoding>();
                  } else {
                      throw KARABO_PYTHON_EXCEPTION(
                            "Python type of setEncoding() of ImageDataElement must be an unsigned integer or string");
                  }
                  return self.setEncoding(encType);
              },
              py::arg("encoding"), py::return_value_policy::reference_internal,
              "h.setEncoding(encoding) set the encoding of an image in the schema. The encoding name must be "
              "a string (all capitals) or Encoding enum.\nExample:\n\t"
              "IMAGEDATA_ELEMENT(s)\n\t\t.key('data.image')\n\t\t.setEncoding('RGB')\n\t\t.commit()");

        el.def(
              "setAllowedActions",
              [](ImageDataElement& self, const py::object& actions) -> ImageDataElement& {
                  // Accept any Python sequence of strings...
                  self.setAllowedActions(wrapper::fromPySequenceToVectorString(actions));
                  return self;
              },
              py::arg("actions"), py::return_value_policy::reference_internal, R"pbdoc(
                Specify one or more actions that are allowed on this node.
                If a Karabo device specifies allowed actions for a node,
                that means that it offers a specific slot interface to operate
                on this node. Which allowed actions require which interface
                is defined elsewhere.
              )pbdoc");

        py::implicitly_convertible<Schema&, ImageDataElement>();
    }
}
