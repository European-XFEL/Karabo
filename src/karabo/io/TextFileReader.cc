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


//namespace exfel {
  //namespace io {

    EXFEL_REGISTER_FACTORY_CC(exfel::io::Reader<exfel::util::Hash>, exfel::io::TextFileReader<exfel::util::Hash> )
    EXFEL_REGISTER_FACTORY_CC(exfel::io::Reader<exfel::util::Schema>, exfel::io::TextFileReader<exfel::util::Schema>)
   
  //} // namespace io
//} // namespace exfel
