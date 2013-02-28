/*
 * $Id: Scalar.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Group.hh"
using namespace karabo::io::h5;

namespace karabo {
  namespace io {
    namespace h5 {
        
       void Group::expectedParameters(karabo::util::Schema& expected) {
           
           
       }
        
      KARABO_REGISTER_FOR_CONFIGURATION(Element, Group)
     
    }
  }
}
