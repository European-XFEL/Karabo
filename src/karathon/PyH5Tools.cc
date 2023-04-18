/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include <boost/python.hpp>
#include <karabo/io/h5/File.hh>

#include "PythonFactoryMacros.hh"

namespace bp = boost::python;
using namespace karabo::util;
using namespace karabo::io;
using std::string;

void exportPyIoH5File() {
    {
        bp::enum_<h5::File::AccessMode>("H5accessMode", "This enumeration define the access mode to file")
              .value("TRUNCATE", h5::File::TRUNCATE)
              .value("READONLY", h5::File::READONLY)
              .value("EXCLUSIVE", h5::File::EXCLUSIVE)
              .value("APPEND", h5::File::APPEND);
    }

    {
        // FormatDiscoveryPolicy
        bp::class_<h5::FormatDiscoveryPolicy> f("H5formatDiscoveryPolicy", bp::no_init);

        f.def("getDefaultCompressionLevel",
              (int(h5::FormatDiscoveryPolicy::*)() const)(&h5::FormatDiscoveryPolicy::getDefaultCompressionLevel));

        f.def("getDefaultChunkSize", (unsigned long long (h5::FormatDiscoveryPolicy::*)()
                                            const)(&h5::FormatDiscoveryPolicy::getDefaultChunkSize));


        f KARABO_PYTHON_FACTORY_CONFIGURATOR(h5::FormatDiscoveryPolicy);

        bp::register_ptr_to_python<boost::shared_ptr<h5::FormatDiscoveryPolicy> >();
    }

    {
        bp::class_<h5::Format> f("H5format", bp::no_init);

        f.def("getConfig", (Hash const &(h5::Format::*)() const)(&h5::Format::getConfig),
              bp::return_value_policy<bp::copy_const_reference>());

        f.def("getPersistentConfig", (Hash const &(h5::Format::*)() const)(&h5::Format::getPersistentConfig),
              bp::return_value_policy<bp::copy_const_reference>());

        f.def("discover", (h5::Format::Pointer(*)(const Hash &))(&h5::Format::discover), bp::arg("hash"),
              "Discover the table format from the hash.");

        f.def("discover",
              (h5::Format::Pointer(*)(const Hash &, h5::FormatDiscoveryPolicy::Pointer))(&h5::Format::discover),
              (bp::arg("hash"), bp::arg("policy")),
              "Discover the table format from the hash using policy for defining chunk Size and compression level")
              .staticmethod("discover");

        f.def("createEmptyFormat", (h5::Format::Pointer(*)())(&h5::Format::createEmptyFormat))
              .staticmethod("createEmptyFormat");

        f.def("addElement", (void(h5::Format::*)(h5::Element::Pointer))(&h5::Format::addElement), bp::arg("element"));

        f.def("removeElement", (void(h5::Format::*)(const std::string &))(&h5::Format::addElement),
              bp::arg("fullPath"));

        f.def("replaceElement",
              (void(h5::Format::*)(const std::string &, h5::Element::Pointer))(&h5::Format::replaceElement),
              (bp::arg("fullPath"), bp::arg("element")));

        f.def("getConstElement",
              (h5::Element::ConstPointer(h5::Format::*)(const std::string &) const)(&h5::Format::getElement),
              bp::arg("fullPath"));

        f.def("getElement", (h5::Element::Pointer(h5::Format::*)(const std::string &))(&h5::Format::getElement),
              bp::arg("fullPath"));

        f KARABO_PYTHON_FACTORY_CONFIGURATOR(h5::Format);

        bp::register_ptr_to_python<boost::shared_ptr<h5::Format> >();
    }

    {
        bp::class_<h5::Table> t("H5table", bp::no_init);

        t.def("write", (void(h5::Table::*)(const Hash &, size_t))(&h5::Table::write), (bp::arg("data"), bp::arg("idx")),
              "Write the hash object as a record in the table at the position idx. "
              "The hash must be compatible with the table definition.");

        t.def("write", (void(h5::Table::*)(const Hash &, size_t, size_t))(&h5::Table::write),
              (bp::arg("data"), bp::arg("idx"), bp::arg("bufferLen")),
              "Write the hash object as a set of records of length bufferLen to the table at position idx. "
              "The hash must be compatible with the table definition. "
              "Take note that the Hash values must be arrays/lists where the first array dimension is equal to the "
              "bufferLen.");

        t.def("append", (void(h5::Table::*)(const Hash &))(&h5::Table::append), (bp::arg("data")),
              "Append the hash object as a record to the end of the table. "
              "The hash must be compatible with the table definition.");

        t.def("bind", (void(h5::Table::*)(Hash &))(&h5::Table::bind), (bp::arg("data")),
              "Bind the Hash to the table. "
              "If the hash object does not contain a key which is defined in the table format "
              "the corresponding key/value pair is created in the hash. If the key/value pair exists it is not "
              "touched. "
              "In most cases you can pass an empty hash to this function. "
              "This function needs to be called before calling read(idx) function. "
              "Read function will fill the hash with requested data record.");

        t.def("bind", (void(h5::Table::*)(Hash &, size_t))(&h5::Table::bind), (bp::arg("data"), bp::arg("bufferLen")),
              "Bind the Hash to the table. "
              "If the hash object does not contain a key which is defined in the table format "
              "the corresponding key/value pair is created in the hash. If the key/value pair exists it is not "
              "touched. "
              "In most cases you can pass an empty hash to this function. "
              "This function needs to be called before calling read(idx, bufferLen) function. "
              "Read function will fill the hash with requested data record.");

        t.def("read", (size_t(h5::Table::*)(size_t))(&h5::Table::read), (bp::arg("idx")),
              "Read data record from the table and fill the hash which has been bound to that table. "
              "Returns the number of read records.");

        t.def("read", (size_t(h5::Table::*)(size_t, size_t))(&h5::Table::read), (bp::arg("idx"), bp::arg("bufferLen")),
              "Read bufferLen number of data records from the table and fill the hash which has been bound to that "
              "table. "
              "Returns the number of read records.");

        t.def("writeAttributes", (void(h5::Table::*)(const Hash &))(&h5::Table::writeAttributes), (bp::arg("data")),
              "Write attributes from the hash object. Only hash attributes are written for the corresponding keys. "
              "Hash values are ignored. ");

        t.def("readAttributes", (void(h5::Table::*)(Hash &))(&h5::Table::readAttributes), (bp::arg("data")),
              "Read attributes from the table as the Hash attributes. Hash values are not filled.");

        t.def("size", (size_t(h5::Table::*)())(&h5::Table::size), "Returns number of records in the table.");

        t.def("getFormat", (h5::Format::Pointer(h5::Table::*)())(&h5::Table::getFormat),
              "Get the format of the table.");

        t.def("getName", (string(h5::Table::*)() const)(&h5::Table::getName), "Get the table name.");

        bp::register_ptr_to_python<boost::shared_ptr<h5::Table> >();
    }

    {
        bp::class_<h5::File, boost::noncopyable> f("H5file", "karabo interface to hdf5",
                                                   bp::init<std::string const &>());

        f.def("open", (void(h5::File::*)(h5::File::AccessMode))(&h5::File::open), bp::arg("accessMode"),
              "Open hdf5 file using given access mode. "
              " Values can be AccessMode.TRUNCUTE, AccessMode.READONLY, AccessMode.EXCLUSIVE, AccessMode.APPEND");

        f.def("isOpen", (bool(h5::File::*)())(&h5::File::isOpen), "Check if the file is open.");

        f.def("hasTable", (bool(h5::File::*)(const string &) const)(&h5::File::hasTable), (bp::arg("path")),
              "Check if the table exists in the file.");
        f.def("close", (void(h5::File::*)())(&h5::File::close), "Close the file and all tables associated with it.");

        f.def("createTable",
              (h5::Table::Pointer(h5::File::*)(const std::string &, const h5::Format::Pointer))(&h5::File::createTable),
              (bp::arg("name"), bp::arg("format")), "Create new H5table within the file.");

        f.def("getTable",
              (h5::Table::Pointer(h5::File::*)(const std::string &, const h5::Format::Pointer, size_t))(
                    &h5::File::getTable),
              (bp::arg("name"), bp::arg("format"), bp::arg("numberOfRecords") = 0),
              "Get the H5table specified by name and use user supplied data format which must be compatible with the "
              "table definition in the file. "
              "This can be useful for reading only selected variables.");

        f.def("getTable", (h5::Table::Pointer(h5::File::*)(const std::string &))(&h5::File::getTable),
              (bp::arg("name")), "Get the H5table specified by name.");

        f.def("getName", (string(h5::File::*)() const)(&h5::File::getName), "Get the file name.");

        f.def("closeTable", (void(h5::File::*)(h5::Table::Pointer))(&h5::File::closeTable), (bp::arg("table")),
              "Close the table.");
    }

    {
        bp::class_<h5::Element, boost::noncopyable> e("H5element", bp::no_init);

        e.def("getFullName", (const string &(h5::Element::*)() const)(&h5::Element::getFullName),
              bp::return_value_policy<bp::copy_const_reference>());

        e.def("getH5path", (const string &(h5::Element::*)() const)(&h5::Element::getH5path),
              bp::return_value_policy<bp::copy_const_reference>());

        e.def("getH5name", (const string &(h5::Element::*)() const)(&h5::Element::getH5name),
              bp::return_value_policy<bp::copy_const_reference>());

        e.def("getKey", (const string (h5::Element::*)(const char) const)(&h5::Element::getKey), bp::arg("sep") = '.');

        e.def("isDataset", (bool(h5::Element::*)() const)(&h5::Element::isDataset));
        e.def("isGroup", (bool(h5::Element::*)() const)(&h5::Element::isGroup));


        e KARABO_PYTHON_FACTORY_CONFIGURATOR(h5::Element);

        bp::register_ptr_to_python<boost::shared_ptr<h5::Element> >();
    }
}