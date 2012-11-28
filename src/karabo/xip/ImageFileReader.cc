/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "ImageFileReader.hh"

namespace karabo {
  namespace xip {

      KARABO_REGISTER_FACTORY_2_CC(karabo::xms::AbstractInput, karabo::xms::Input<CpuImage<float> >, ImageFileReader<float>)
      KARABO_REGISTER_FACTORY_CC(karabo::xms::Input<CpuImage<float> >, ImageFileReader<float>)
      
      
  } 
} 
