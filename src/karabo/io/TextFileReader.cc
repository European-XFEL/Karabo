/*
 * $Id: TextFileReader.cc 4972 2012-01-13 10:17:15Z wegerk@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 16, 2011, 8:49 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "TextFileReader.hh"


//namespace karabo {
  //namespace io {

    KARABO_REGISTER_FACTORY_CC(karabo::io::Reader<karabo::util::Hash>, karabo::io::TextFileReader<karabo::util::Hash> )
    KARABO_REGISTER_FACTORY_CC(karabo::io::Reader<karabo::util::Schema>, karabo::io::TextFileReader<karabo::util::Schema>)
   
  //} // namespace io
//} // namespace karabo
