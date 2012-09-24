/*
 * $Id: Group.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef EXFEL_IO_GROUP_HH
#define	EXFEL_IO_GROUP_HH

#include <karabo/util/Factory.hh>
#include "../iodll.hh"

#include <string>

namespace exfel {
  namespace io {


    class Group {
    public:
      EXFEL_CLASSINFO(Group, "Group", "1.0")
      EXFEL_FACTORY_BASE_CLASS


      virtual ~Group(){
      }
      
      inline const std::string& getName() const {
        return m_name;
      }
      
      inline const std::string& getPath() const {
        return m_path;
      }

      static void expectedParameters(exfel::util::Schema& expected);
      void configure(const exfel::util::Hash& input);
    
    private:

      std::string m_name;
      std::string m_path;

    };
  }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::Group, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* EXFEL_IO_GROUP_HH */

