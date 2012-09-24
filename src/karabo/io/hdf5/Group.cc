/*
 * $Id: Group.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Group.hh"
#include "Scalar.hh"


using namespace std;
using namespace exfel::util;
using namespace boost;

namespace exfel {
  namespace io {

    EXFEL_REGISTER_ONLY_ME_CC(Group)

    void Group::expectedParameters(Schema& expected) {

      STRING_ELEMENT(expected)
              .key("name")
              .displayedName("Name")
              .description("Group Name")
              .assignmentMandatory()
              .reconfigurable()
              .commit();

      STRING_ELEMENT(expected)
              .key("path")
              .displayedName("Path")
              .description("Relative path to the group")
              .assignmentMandatory()
              .reconfigurable()
              .commit();

    }

    void Group::configure(const Hash& input) {
      m_name = input.get<string > ("name");
      m_path = input.get<string> ("path");
      
      if( m_path.size() > 1 ){
          trim_right_if(m_path, is_any_of("/"));
      }

    }

  }
}
