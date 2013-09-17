/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include <boost/python.hpp>

#include <karabo/io/h5/File.hh>
#include "PythonFactoryMacros.hh"

namespace bp = boost::python;
using namespace karabo::util;
using namespace karabo::io;


void exportPyIoH5File() {

    {
        bp::enum_<h5::File::AccessMode>("AccessMode", "This enumeration define the access mode to file")
                .value("TRUNCATE", h5::File::TRUNCATE)
                .value("READONLY", h5::File::READONLY)
                .value("EXCLUSIVE", h5::File::EXCLUSIVE)
                .value("APPEND", h5::File::APPEND)
                ;

    }

    {
        //FormatDiscoveryPolicy
        bp::class_<h5::FormatDiscoveryPolicy> f("H5formatDiscoveryPolicy", bp::no_init);

        f.def("getDefaultCompressionLevel"
              , (int (h5::FormatDiscoveryPolicy::*)() const) (&h5::FormatDiscoveryPolicy::getDefaultCompressionLevel)
              );

        f.def("getDefaulthunkSize"
              , (unsigned long long (h5::FormatDiscoveryPolicy::*)() const) (&h5::FormatDiscoveryPolicy::getDefaultChunkSize)
              );


        f
        KARABO_PYTHON_FACTORY_CONFIGURATOR(h5::FormatDiscoveryPolicy);

        bp::register_ptr_to_python< boost::shared_ptr<h5::FormatDiscoveryPolicy> >();        
       
    }

    {
        bp::class_<h5::Format> f("H5format", bp::no_init);

        f.def("getConfig"
              , (Hash const & (h5::Format::*)() const) (&h5::Format::getConfig)
              , bp::return_value_policy<bp::copy_const_reference > ()
              );

        f.def("getPersistentConfig"
              , (Hash const & (h5::Format::*)() const) (&h5::Format::getPersistentConfig)
              , bp::return_value_policy<bp::copy_const_reference > ()
              );
 
        f.def("discover"
              , (h5::Format::Pointer(*)(const Hash &)) (&h5::Format::discover)
              , bp::arg("hash")
              );

        f.def("discover"
              , (h5::Format::Pointer(*)(const Hash &, h5::FormatDiscoveryPolicy::Pointer)) (&h5::Format::discover)
              , (bp::arg("hash"), bp::arg("policy"))
              ).staticmethod("discover");

        f.def("createEmptyFormat"
              , (h5::Format::Pointer(*)()) (&h5::Format::createEmptyFormat)
              ).staticmethod("createEmptyFormat");

        f.def("addElement"
              , (void(h5::Format::*)(h5::Element::Pointer)) (&h5::Format::addElement)
              , bp::arg("element")
              );

        f.def("removeElement"
              , (void (h5::Format::*)(const std::string&)) (&h5::Format::addElement)
              , bp::arg("fullPath")
              );

        f.def("replaceElement"
              , (void (h5::Format::*)(const std::string&, h5::Element::Pointer)) (&h5::Format::replaceElement)
              , (bp::arg("fullPath"), bp::arg("element"))
              );

        f.def("getConstElement"
              , (h5::Element::ConstPointer(h5::Format::*)(const std::string&) const) (&h5::Format::getElement)
              , bp::arg("fullPath")
              );

        f.def("getElement"
              , (h5::Element::Pointer(h5::Format::*)(const std::string&)) (&h5::Format::getElement)
              , bp::arg("fullPath")
              );

        f
        KARABO_PYTHON_FACTORY_CONFIGURATOR(h5::Format);

        bp::register_ptr_to_python< boost::shared_ptr<h5::Format> >();
    }

    {

        bp::class_<h5::Table> t("H5table", bp::no_init);

        t.def("write"
              , (void(h5::Table::*)(const Hash &, size_t)) (&h5::Table::write)
              , (bp::arg("data"), bp::arg("idx"))
              );

        t.def("write"
              , (void(h5::Table::*)(const Hash &, size_t, size_t)) (&h5::Table::write)
              , (bp::arg("data"), bp::arg("idx"), bp::arg("bufferLen"))
              );

        t.def("append"
              , (void(h5::Table::*)(const Hash &)) (&h5::Table::append)
              , (bp::arg("data"))
              );

        t.def("bind"
              , (void(h5::Table::*)(Hash &)) (&h5::Table::bind)
              , (bp::arg("data"))
              );

        t.def("bind"
              , (void(h5::Table::*)(Hash &, size_t)) (&h5::Table::bind)
              , (bp::arg("data"), bp::arg("bufferLen"))
              );

        t.def("read"
              , (size_t(h5::Table::*)(size_t)) (&h5::Table::read)
              , (bp::arg("idx"))
              );

        t.def("read"
              , (size_t(h5::Table::*)(size_t, size_t)) (&h5::Table::read)
              , (bp::arg("idx"), bp::arg("bufferLen"))
              );

        t.def("writeAttributes"
              , (void(h5::Table::*)(const Hash &)) (&h5::Table::writeAttributes)
              , (bp::arg("data"))
              );

        t.def("readAttributes"
              , (void(h5::Table::*)(Hash &)) (&h5::Table::readAttributes)
              , (bp::arg("data"))
              );

        t.def("size"
              , (size_t(h5::Table::*)())(&h5::Table::size)
              );

        t.def("getFormat"
              , (h5::Format::Pointer(h5::Table::*)())(&h5::Table::getFormat)
              );

        t.def("getName"
              , (string(h5::Table::*)() const) (&h5::Table::getName)
              );

        bp::register_ptr_to_python< boost::shared_ptr<h5::Table> >();

    }

    {
        bp::class_<h5::File, boost::noncopyable > f("H5file", bp::init< std::string const &>());

        f.def("open"
              , (void (h5::File::*)(h5::File::AccessMode)) (&h5::File::open)
              , bp::arg("accessMode")
              );

        f.def("isOpen"
              , (bool (h5::File::*)()) (&h5::File::isOpen)
              );

        f.def("close"
              , (void (h5::File::*)()) (&h5::File::close)
              );

        f.def("createTable"
              , (h5::Table::Pointer(h5::File::*)(const std::string&, const h5::Format::Pointer)) (&h5::File::createTable)
              , (bp::arg("name"), bp::arg("format"))
              );

        f.def("getTable"
              , (h5::Table::Pointer(h5::File::*)(const std::string&, const h5::Format::Pointer, size_t)) (&h5::File::getTable)
              , (bp::arg("name"), bp::arg("format"), bp::arg("numberOfRecords") = 0)
              );

        f.def("getTable"
              , (h5::Table::Pointer(h5::File::*)(const std::string&)) (&h5::File::getTable)
              , (bp::arg("name"))
              );

        f.def("getName"
              , (string(h5::Table::*)() const) (&h5::File::getName)
              );

        f.def("closeTable"
              , (void(h5::File::*)(h5::Table::Pointer)) (&h5::File::closeTable)
              , (bp::arg("table"))
              );


    }

    {

        bp::class_<h5::Element, boost::noncopyable > e("H5element", bp::no_init);

        e.def("getFullName"
              , (const string & (h5::Element::*)() const) (&h5::Element::getFullName)
              , bp::return_value_policy<bp::copy_const_reference > ()
              );

        e.def("getH5path"
              , (const string & (h5::Element::*)() const) (&h5::Element::getH5path)
              , bp::return_value_policy<bp::copy_const_reference > ()
              );

        e.def("getH5name"
              , (const string & (h5::Element::*)() const) (&h5::Element::getH5name)
              , bp::return_value_policy<bp::copy_const_reference > ()
              );

        e.def("getKey"
              , (const string(h5::Element::*)(const char) const) (&h5::Element::getKey)
              , bp::arg("sep") = '.'
              );

        e.def("isDataset"
              , (bool(h5::Element::*)() const) (&h5::Element::isDataset)
              );
        e.def("isGroup"
              , (bool(h5::Element::*)() const) (&h5::Element::isGroup)
              );



        e
        KARABO_PYTHON_FACTORY_CONFIGURATOR(h5::Element);

        bp::register_ptr_to_python< boost::shared_ptr<h5::Element> >();


    }
}