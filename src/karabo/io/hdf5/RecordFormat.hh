/*
 * $Id: RecordFormat.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef KARABO_IO_RECORDFORMAT_HH
#define	KARABO_IO_RECORDFORMAT_HH

#include <vector>
#include <boost/shared_ptr.hpp>
#include <karabo/util/Factory.hh>
#include <karabo/util/Hash.hh>
#include "RecordElement.hh"
#include "Group.hh"
#include "DataBlock.hh"
#include "../iodll.hh"

#include <string>

namespace karabo {
  namespace io {
    namespace hdf5 {
      class DataFormat;

      class RecordFormat {
        friend class DataFormat;
      public:

        KARABO_CLASSINFO(RecordFormat, "RecordFormat", "1.0")
        KARABO_FACTORY_BASE_CLASS

      private:
        std::vector<Group::Pointer > m_groupList;
        std::string m_root;
        karabo::util::Hash m_config;

        karabo::util::Hash m_recordElementHash;


        void buildRecordFormat(const std::vector<DataBlock::Pointer>& dataBlockList);

      public:

        virtual ~RecordFormat() {
        }


        static void expectedParameters(karabo::util::Schema& expected);
        void configure(const karabo::util::Hash& input);

        void getHash(karabo::util::Hash&);
        karabo::util::Hash getConfig();

      };
    }
  }
}

KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::hdf5::RecordFormat, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* KARABO_IO_RECORDFORMAT_HH */

