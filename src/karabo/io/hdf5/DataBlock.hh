/*
 * $Id: DataBlock.hh 5395 2012-03-07 16:10:07Z wegerk $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef KARABO_IO_DATABLOCK_HH
#define	KARABO_IO_DATABLOCK_HH

#include "RecordElement.hh"

#include <string>

namespace karabo {
  namespace io {


    class DataBlock {
    public:
      KARABO_CLASSINFO(DataBlock, "DataBlock", "1.0")
      KARABO_FACTORY_BASE_CLASS


      virtual ~DataBlock(){
      }
      
      static void expectedParameters(karabo::util::Schema& expected);
      void configure(const karabo::util::Hash& input);
      
      
      /*
       * Get the name of the DataBlock
       */
      inline const std::string& getName() const {
        return m_name;
      }
      
      /*
       *  Get Hash representation of the DataBlock
       */ 
      void getHash(karabo::util::Hash&) const;
     
      /*
       * Get the Hash object used for creation
       */
      karabo::util::Hash getConfig() const;
      
      /*
       * Make a deep copy of the original DataBlock.
       */
      DataBlock::Pointer duplicate() const;
    
    
    private:
      std::string m_name;
      std::vector<karabo::io::RecordElement::Pointer > m_elementList;
      karabo::util::Hash m_config;
      

    };
  }
}

KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::DataBlock, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* KARABO_IO_DATABLOCK_HH */

