/*
 * $Id: DataBlock.hh 5395 2012-03-07 16:10:07Z wegerk $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef EXFEL_IO_DATABLOCK_HH
#define	EXFEL_IO_DATABLOCK_HH

#include "RecordElement.hh"

#include <string>

namespace exfel {
  namespace io {


    class DataBlock {
    public:
      EXFEL_CLASSINFO(DataBlock, "DataBlock", "1.0")
      EXFEL_FACTORY_BASE_CLASS


      virtual ~DataBlock(){
      }
      
      static void expectedParameters(exfel::util::Schema& expected);
      void configure(const exfel::util::Hash& input);
      
      
      /*
       * Get the name of the DataBlock
       */
      inline const std::string& getName() const {
        return m_name;
      }
      
      /*
       *  Get Hash representation of the DataBlock
       */ 
      void getHash(exfel::util::Hash&) const;
     
      /*
       * Get the Hash object used for creation
       */
      exfel::util::Hash getConfig() const;
      
      /*
       * Make a deep copy of the original DataBlock.
       */
      DataBlock::Pointer duplicate() const;
    
    
    private:
      std::string m_name;
      std::vector<exfel::io::RecordElement::Pointer > m_elementList;
      exfel::util::Hash m_config;
      

    };
  }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::DataBlock, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* EXFEL_IO_DATABLOCK_HH */

