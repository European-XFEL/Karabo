/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>

#include <karabo/io/SchemaXsdSerializer.hh>
#include <karabo/io/SchemaXmlSerializer.hh>
#include <karabo/io/HashXmlSerializer.hh>
#include "PythonFactoryMacros.hh"

using namespace karabo::util;
using namespace karabo::io;
using namespace std;
namespace bp = boost::python;


namespace karabo {
    namespace io {
        
        class HashXmlSerializerWrap {
        public:
            
            static bp::object save(HashXmlSerializer& s, const karabo::util::Hash& hash) {
                std::string archive;
                s.save(hash, archive);
                return bp::object(archive);
            }
            
            static bp::object load(HashXmlSerializer& s, const std::string& archive) {
                Hash hash;
                s.load(hash, archive);
                return bp::object(hash);
            }
        };
    }
}


void exportPyIoSerialization() {

    {//exposing karabo::io::HashXmlSerializer

        bp::class_< HashXmlSerializer > h("HashXmlSerializer", bp::init<Hash const &>((bp::arg("hash"))));
            h.def("save"
              , &HashXmlSerializerWrap().save
              , (bp::arg("hash")));
            h.def("load"
              , &HashXmlSerializerWrap().load
              , (bp::arg("archive")));
    }

    {//exposing karabo::io::SchemaXsdSerializer

        bp::class_< SchemaXsdSerializer > s("SchemaXsdSerializer", bp::init<Hash const &>((bp::arg("input"))));
            s.def("save"
              , (void (SchemaXsdSerializer::*)(Schema const &, string &))(&SchemaXsdSerializer::save)
              , (bp::arg("object"), bp::arg("archive")));
    }

    {//exposing karabo::io::SchemaXmlSerializer

        bp::class_< SchemaXmlSerializer > s("SchemaXmlSerializer", bp::init<Hash const &>((bp::arg("hash"))));
            s.def("save"
              , (void (SchemaXmlSerializer::*)(Schema const &, string &))(&SchemaXmlSerializer::save)
              , (bp::arg("object"), bp::arg("archive")));
    }


}
