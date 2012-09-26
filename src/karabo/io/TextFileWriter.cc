/*
 * $Id: TextFileWriter.cc 4951 2012-01-06 12:54:57Z heisenb@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 16, 2010, 10:18 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "TextFileWriter.hh"

//namespace karabo {
  //namespace io {

    KARABO_REGISTER_FACTORY_CC(karabo::io::Writer<karabo::util::Hash>, karabo::io::TextFileWriter<karabo::util::Hash>)
    KARABO_REGISTER_FACTORY_CC(karabo::io::Writer<karabo::util::Schema>, karabo::io::TextFileWriter<karabo::util::Schema>)
            
	//} // namespace io
//} // namespace karabo
