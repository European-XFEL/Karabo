/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "ImageFileWriter.hh"

namespace karabo {
  namespace xip {

      //KARABO_REGISTER_FACTORY_CC(Output<CpuImgF >, ImageFileWriter<float>)
      //KARABO_REGISTER_FACTORY_CC(Output<CpuImgD >, ImageFileWriter<double>)
      
      KARABO_REGISTER_FACTORY_2_CC(karabo::xms::AbstractOutput, karabo::xms::Output<CpuImage<float> >, ImageFileWriter<float>)
      KARABO_REGISTER_FACTORY_CC(karabo::xms::Output<CpuImage<float> >, ImageFileWriter<float>)
              
      KARABO_REGISTER_FACTORY_2_CC(karabo::xms::AbstractOutput, karabo::xms::Output<CpuImage<double> >, ImageFileWriter<double>)
      KARABO_REGISTER_FACTORY_CC(karabo::xms::Output<CpuImage<double> >, ImageFileWriter<double>)
      
  } 
} 
