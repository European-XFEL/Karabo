/*
 * $Id: DataBlock.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "DataBlock.hh"


using namespace std;
using namespace karabo::util;

namespace karabo {
  namespace io {

    KARABO_REGISTER_ONLY_ME_CC(DataBlock)

    void DataBlock::expectedParameters(Schema& expected) {

      STRING_ELEMENT(expected)
              .key("name")
              .displayedName("Name")
              .description("Family name")
              .assignmentMandatory()
              .reconfigurable()
              .commit();

      NON_EMPTY_LIST_ELEMENT<RecordElement > (expected)
              .key("elements")
              .displayedName("Record Elements")
              .description("Definition of record format. Non empty list of record elements")
              .assignmentMandatory()
              .reconfigurable()
              .commit();

    }

    void DataBlock::configure(const Hash& input) {
      m_config = input;
      m_name = input.get<string > ("name");
      m_elementList = RecordElement::createList("elements", input);
    }
    
    DataBlock::Pointer DataBlock::duplicate() const {
        boost::shared_ptr<DataBlock> copy( new DataBlock() );
        copy->configure(m_config);
        return copy;
    }

    void DataBlock::getHash(Hash& recordFormatHash) const {
      recordFormatHash.clear();
      for (size_t i = 0; i < m_elementList.size(); ++i) {
        m_elementList[i]->getElement(recordFormatHash);
      }
    }

    Hash DataBlock::getConfig() const {
      return Hash("elements",m_config);
    }




  }
}
