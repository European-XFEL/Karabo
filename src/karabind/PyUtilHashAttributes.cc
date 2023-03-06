#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/stl_bind.h>
#include <karabo/util/FromLiteral.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include <string>

#include "PyTypes.hh"
#include "Wrapper.hh"

namespace py = pybind11;

using namespace karabo::util;


namespace karabind {

    struct null_deleter {
        void operator()(void const*) const {}
    };

    class HashAttributesNodeHelper {
       public:
        static py::object toObject(const Hash::Attributes::Node& self, const Types::ReferenceType& type) {
            switch (type) {
                case Types::BOOL:
                    return py::cast(self.getValueAs<bool>());
                case Types::CHAR:
                    return py::cast(self.getValueAs<char>());
                case Types::INT8:
                    return py::cast(self.getValueAs<signed char>());
                case Types::UINT8:
                    return py::cast(self.getValueAs<unsigned char>());
                case Types::INT16:
                    return py::cast(self.getValueAs<short>());
                case Types::UINT16:
                    return py::cast(self.getValueAs<unsigned short>());
                case Types::INT32:
                    return py::cast(self.getValueAs<int>());
                case Types::UINT32:
                    return py::cast(self.getValueAs<unsigned int>());
                case Types::INT64:
                    return py::cast(self.getValueAs<long long>());
                case Types::UINT64:
                    return py::cast(self.getValueAs<unsigned long long>());
                case Types::FLOAT:
                    return py::cast(self.getValueAs<float>());
                case Types::DOUBLE:
                    return py::cast(self.getValueAs<double>());
                case Types::COMPLEX_FLOAT:
                {
                    auto val = self.getValueAs<std::complex<float>>();
                    return py::cast(val);
                }
                case Types::COMPLEX_DOUBLE:
                {
                    auto val = self.getValueAs<std::complex<double>>();
                    return py::cast(val);
                }
                case Types::STRING:
                    return py::cast(self.getValueAs<std::string>());
                case Types::VECTOR_BOOL:
                    return py::cast(self.getValueAs<bool, std::vector>());
                case Types::VECTOR_CHAR:
                    return py::cast(self.getValueAs<char, std::vector>());
                case Types::VECTOR_INT8:
                    return py::cast(self.getValueAs<signed char, std::vector>());
                case Types::VECTOR_UINT8:
                    return py::cast(self.getValueAs<unsigned char, std::vector>());
                case Types::VECTOR_INT16:
                    return py::cast(self.getValueAs<short, std::vector>());
                case Types::VECTOR_UINT16:
                    return py::cast(self.getValueAs<unsigned short, std::vector>());
                case Types::VECTOR_INT32:
                    return py::cast(self.getValueAs<int, std::vector>());
                case Types::VECTOR_UINT32:
                    return py::cast(self.getValueAs<unsigned int, std::vector>());
                case Types::VECTOR_INT64:
                    return py::cast(self.getValueAs<long long, std::vector>());
                case Types::VECTOR_UINT64:
                    return py::cast(self.getValueAs<unsigned long long, std::vector>());
                case Types::VECTOR_FLOAT:
                    return py::cast(self.getValueAs<float, std::vector>());
                case Types::VECTOR_DOUBLE:
                    return py::cast(self.getValueAs<double, std::vector>());
                case Types::VECTOR_COMPLEX_FLOAT:
                {
                    auto val = self.getValueAs<std::complex<float>, std::vector>();
                    return py::cast(val);
                }
                case Types::VECTOR_COMPLEX_DOUBLE:
                {
                    auto val = self.getValueAs<std::complex<double>, std::vector>();
                    return py::cast(val);
                }
                default:
                    break;
            }
            std::ostringstream oss;
            oss << "Type " << int(type) << " is not yet supported";
            throw KARABO_NOT_SUPPORTED_EXCEPTION(oss.str());
        }
    };

} // namespace karabind


PYBIND11_DECLARE_HOLDER_TYPE(T, boost::shared_ptr<T>);


void exportPyUtilHashAttributes(py::module_& m) {
    using namespace karabo::util;
    using namespace karabind;

    py::class_<Hash::Attributes::Node, boost::shared_ptr<Hash::Attributes::Node>> an(m, "HashAttributesNode");

    an.def(
          "getKey", [](const Hash::Attributes::Node& self) -> std::string { return self.getKey(); },
          "Get key of current node in attribute's container");

    an.def("__str__", [](const Hash::Attributes::Node& self) -> std::string { return self.getKey(); });

    an.def(
          "setValue", [](Hash::Attributes::Node& self, const py::object& o) { self.setValue(o); }, py::arg("value"),
          "Set value for current node in attribute's container");

    an.def(
          "getValue", [](const Hash::Attributes::Node& self) { return wrapper::castAnyToPy(self.getValueAsAny()); },
          "Get value for current node in attribute's container");

    an.def(
          "getValueAs",
          [](const Hash::Attributes::Node& self, const py::object& otype) {
              using namespace karabo::util;
              Types::ReferenceType type = Types::UNKNOWN;
              if (py::isinstance<py::str>(otype)) {
                  const std::string& stype = otype.cast<std::string>();
                  type = Types::from<FromLiteral>(stype);
              } else if (py::isinstance<PyTypes::ReferenceType>(otype)) {
                  PyTypes::ReferenceType ptype = otype.cast<PyTypes::ReferenceType>();
                  type = PyTypes::to(ptype);
              }
              return HashAttributesNodeHelper::toObject(self, type);
          },
          py::arg("type"), "Get value as a type given as an argument for current node");

    an.def(
          "getType",
          [](const Hash::Attributes::Node& node) {
              using namespace karabo::util;
              PyTypes::ReferenceType type = static_cast<PyTypes::ReferenceType>(node.getType());
              return py::cast(type);
          },
          "Get type of the value kept in current node");

    an.def(
          "setType",
          [](Hash::Attributes::Node& node, const py::object& otype) {
              using namespace karabo::util;
              Types::ReferenceType type = Types::UNKNOWN;
              if (py::isinstance<py::str>(otype)) {
                  std::string stype = otype.cast<std::string>();
                  type = Types::from<FromLiteral>(stype);
              } else if (py::isinstance<PyTypes::ReferenceType>(otype)) {
                  PyTypes::ReferenceType ptype = otype.cast<PyTypes::ReferenceType>();
                  type = PyTypes::to(ptype);
              }
              node.setType(type);
          },
          py::arg("type"), "Set type for value kept in current node");


    py::class_<Hash::Attributes> a(
          m, "HashAttributes",
          "The HashAttributes class is a heterogeneous container with string key and \"any object\" value\n"
          "that preserves insertion order, i.e. it is behaving like an ordered map");
    a.def(py::init<>());

    a.def(
          "has",
          [](karabo::util::Hash::Attributes& self, const std::string& key) -> py::bool_ { return self.has(key); },
          (py::arg("key")), "Returns True if HashAttributes container contains given \"key\"");

    a.def(
          "__contains__",
          [](karabo::util::Hash::Attributes& self, const std::string& key) -> py::bool_ { return self.has(key); },
          py::arg("key"), "Returns True if HashAttributes container contains given \"key\"");

    a.def(
          "isType",
          [](karabo::util::Hash::Attributes& self, const std::string& key, const py::object& otype) -> py::bool_ {
              using namespace karabo::util;
              auto node = self.getNode(key);
              Types::ReferenceType type = node.getType();
              if (py::isinstance<py::str>(otype)) {
                  std::string stype = otype.cast<std::string>();
                  return (type == Types::from<FromLiteral>(stype));
              }
              if (py::isinstance<PyTypes::ReferenceType>(otype)) {
                  PyTypes::ReferenceType ptype = otype.cast<PyTypes::ReferenceType>();
                  return (type == PyTypes::to(ptype));
              }
              return false;
          },
          py::arg("key"), py::arg("type"),
          "Returns True if HashAttributes container has given \"key\" of reference \"type\"..");

    a.def(
          "getType",
          [](karabo::util::Hash::Attributes& self, const std::string& key) {
              using namespace karabo::util;
              auto node = self.getNode(key);
              PyTypes::ReferenceType type = static_cast<PyTypes::ReferenceType>(node.getType());
              return py::cast(type);
          },
          py::arg("key"), "Returns ReferenceType for given attribute \"key\"");

    a.def(
          "erase", [](karabo::util::Hash::Attributes& self, const std::string& key) { self.erase(key); },
          py::arg("key"), "Erase \"key\" attribute");

    a.def(
          "__delitem__", [](karabo::util::Hash::Attributes& self, const std::string& key) { self.erase(key); },
          py::arg("key"), "Erase \"key\" attribute");

    a.def(
          "size", [](karabo::util::Hash::Attributes& self) { return self.size(); },
          "Returns number of entries in HashAttributes container");

    a.def(
          "__len__", [](karabo::util::Hash::Attributes& self) { return self.size(); },
          "Returns number of entries in HashAttributes container");

    a.def(
          "empty", [](karabo::util::Hash::Attributes& self) { return self.empty(); },
          "Returns True if HashAttributes container is empty.");

    a.def(
          "bool", [](karabo::util::Hash::Attributes& self) { return self.size() > 0; },
          "This function automatically called when HashAttributes object checked in \"if\" expression. \"False\" means "
          "that container is empty.");

    a.def(
          "clear", [](karabo::util::Hash::Attributes& self) { self.clear(); }, "Make HashAttributes container empty.");

    a.def(
          "getNode",
          [](karabo::util::Hash::Attributes& self, const std::string& key) {
              using namespace karabo::util;
              Hash::Attributes::Node& nodeRef = self.getNode(key);
              boost::optional<Hash::Attributes::Node&> node(nodeRef);
              return py::cast(boost::shared_ptr<Hash::Attributes::Node>(&(*node), [](const void*) {}));
          },
          py::arg("key"), "Returns HashAttributesNode object associated with \"key\" attribute.");

    a.def(
          "get",
          [](karabo::util::Hash::Attributes& self, const std::string& key) {
              return wrapper::castAnyToPy(self.getAny(key));
          },
          py::arg("key"), "Returns value for \"key\" attribute.");

    a.def(
          "__getitem__",
          [](karabo::util::Hash::Attributes& self, const std::string& key) {
              return wrapper::castAnyToPy(self.getAny(key));
          },
          py::arg("key"), "Pythonic style for getting value of attribute: x = attrs['abc']");

    a.def(
          "getAs",
          [](karabo::util::Hash::Attributes& self, const std::string& key, const py::object& otype) {
              using namespace karabo::util;
              Types::ReferenceType rtype = Types::UNKNOWN;
              if (py::isinstance<py::str>(otype)) {
                  std::string stype = otype.cast<std::string>();
                  rtype = Types::from<FromLiteral>(stype);
              } else if (py::isinstance<PyTypes::ReferenceType>(otype)) {
                  PyTypes::ReferenceType type = otype.cast<PyTypes::ReferenceType>();
                  rtype = PyTypes::to(type);
              }
              const auto& node = self.getNode(key);
              return HashAttributesNodeHelper::toObject(node, rtype);
          },
          py::arg("key"), py::arg("type"), "Get the value of the \"key\" attribute and convert it to type \"type\".");

    a.def(
          "set",
          [](karabo::util::Hash::Attributes& self, const std::string& key, const py::object& value) {
              boost::any a;
              wrapper::castPyToAny(value, a);
              return py::cast(self.set(key, a));
          },
          py::arg("key"), py::arg("value"), "Set the \"value\" for \"key\" attribute.");

    a.def(
          "__setitem__",
          [](karabo::util::Hash::Attributes& self, const std::string& key, const py::object& value) {
              boost::any a;
              wrapper::castPyToAny(value, a);
              return py::cast(self.set(key, a));
          },
          py::arg("key"), py::arg("value"), "Pythonic style for setting value of attribute: attrs['abc'] = 123");

    a.def(
          "__iter__", [](Hash::Attributes& self) { return py::make_iterator(self.begin(), self.end()); },
          py::keep_alive<0, 1>());
}
