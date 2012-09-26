/*
 * $Id: Group.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef KARABO_IO_GROUP_HH
#define	KARABO_IO_GROUP_HH

#include <karabo/util/Factory.hh>
#include "../iodll.hh"

#include <string>

namespace karabo {
  namespace io {


    class Group {
    public:
      KARABO_CLASSINFO(Group, "Group", "1.0")
      KARABO_FACTORY_BASE_CLASS


      virtual ~Group(){
      }
      
      inline const std::string& getName() const {
        return m_name;
      }
      
      inline const std::string& getPath() const {
        return m_path;
      }

      static void expectedParameters(karabo::util::Schema& expected);
      void configure(const karabo::util::Hash& input);
    
    private:

      std::string m_name;
      std::string m_path;

    };
  }
}

KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::Group, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* KARABO_IO_GROUP_HH */

