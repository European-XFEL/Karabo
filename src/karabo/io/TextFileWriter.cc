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

//namespace exfel {
  //namespace io {

    EXFEL_REGISTER_FACTORY_CC(exfel::io::Writer<exfel::util::Hash>, exfel::io::TextFileWriter<exfel::util::Hash>)
    EXFEL_REGISTER_FACTORY_CC(exfel::io::Writer<exfel::util::Schema>, exfel::io::TextFileWriter<exfel::util::Schema>)
            
	//} // namespace io
//} // namespace exfel
