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


void exportPyIoSerialization() {

    {//exposing karabo::io::HashXmlSerializer

        bp::class_< HashXmlSerializer > h("HashXmlSerializer", bp::init<Hash const &>((bp::arg("hash"))));
            h.def("save"
              , (void (HashXmlSerializer::*)(Hash const &, string &))(&HashXmlSerializer::save)
              , (bp::arg("object"), bp::arg("archive")));
            h.def("load"
              , (void (HashXmlSerializer::*)(Hash &, string const &))(&HashXmlSerializer::load)
              , (bp::arg("object"), bp::arg("archive")));
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
