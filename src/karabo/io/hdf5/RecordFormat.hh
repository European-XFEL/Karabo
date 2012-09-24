/*
 * $Id: RecordFormat.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef EXFEL_IO_RECORDFORMAT_HH
#define	EXFEL_IO_RECORDFORMAT_HH

#include <vector>
#include <boost/shared_ptr.hpp>
#include <karabo/util/Factory.hh>
#include <karabo/util/Hash.hh>
#include "RecordElement.hh"
#include "Group.hh"
#include "DataBlock.hh"
#include "../iodll.hh"

#include <string>

namespace exfel {
  namespace io {
    namespace hdf5 {
      class DataFormat;

      class RecordFormat {
        friend class DataFormat;
      public:

        EXFEL_CLASSINFO(RecordFormat, "RecordFormat", "1.0")
        EXFEL_FACTORY_BASE_CLASS

      private:
        std::vector<Group::Pointer > m_groupList;
        std::string m_root;
        exfel::util::Hash m_config;

        exfel::util::Hash m_recordElementHash;


        void buildRecordFormat(const std::vector<DataBlock::Pointer>& dataBlockList);

      public:

        virtual ~RecordFormat() {
        }


        static void expectedParameters(exfel::util::Schema& expected);
        void configure(const exfel::util::Hash& input);

        void getHash(exfel::util::Hash&);
        exfel::util::Hash getConfig();

      };
    }
  }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::hdf5::RecordFormat, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* EXFEL_IO_RECORDFORMAT_HH */

