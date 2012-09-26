/*
 * $Id: RecordFormat.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "RecordFormat.hh"
#include "RecordElement.hh"


using namespace std;
using namespace karabo::util;
using namespace boost;

namespace karabo {
  namespace io {
    namespace hdf5 {

      KARABO_REGISTER_ONLY_ME_CC(RecordFormat)

      void RecordFormat::expectedParameters(Schema& expected) {

        NON_EMPTY_LIST_ELEMENT<Group > (expected)
                .key("groups")
                .displayedName("Groups")
                .description("Definition of record format. Non empty list of data blocks.")
                .assignmentMandatory()
                .reconfigurable()
                .commit();

        STRING_ELEMENT(expected)
                .key("root")
                .displayedName("Root Element")
                .description("Root element of the record. Each data block is rooted here.")
                .assignmentOptional().defaultValue("")
                .commit();

      }

      void RecordFormat::configure(const Hash& input) {
        m_config = input;
        m_groupList = Group::createList("groups", input);
        m_root = input.get<string > ("root");
      }

      void RecordFormat::buildRecordFormat(const vector<DataBlock::Pointer>& dataBlockList) {
        // Build record from data blocks. This function can only be called by DataFormat object.
        // One data block can be used zero to N times
        // Every time it is used in the record the deep copy is made (because of pointer)
        // The ouptput is a Hash with corresponding data structres.
        // Record defines 1-to-1 relation between RecordElements defined within each DataBlock
        // Record is similar to the database table record but may contains nested fields (always 1-to-1 relation)
        // ie: a, b, d/e, g, h/j/k, h/j/l
        // but some Hash'es are not supported. 
        // DataBlock name cannot be used as last part of the path
        // i.e. this Hash will not work:  
        // a.b.x    => path is "/a", DataBlock is b, x is a value of any type
        // a.b.e.y  => path is "/a/b" !!!, DataBlock is e, y is value 
        //
        // DataBlock names are always used as hdf5 groups
        //
        // TODO: re-think if we need to support this funny case.

        // create hash with key equal to the dataBlock name, value equal to DataBlock pointer 
        Hash dataBlocks;
        for (size_t i = 0; i < dataBlockList.size(); ++i) {
          const string& key = dataBlockList[i]->getName();
          //cerr << "key: " << key << endl;
          dataBlocks.set(key, dataBlockList[i]);
        }

        // for each datablock defined in Group make a copy of data block and put it to the Hash
        for (size_t i = 0; i < m_groupList.size(); ++i) {
          const string groupName = m_groupList[i]->getName();
          //cerr << "groupName: " << groupName << endl;

          DataBlock::Pointer dataBlock = dataBlocks.get<DataBlock::Pointer > (groupName)->duplicate();
          Hash dataBlockHash;
          dataBlock->getHash(dataBlockHash);

          string path = m_groupList[i]->getPath();
          //cerr << "path: " << path << endl;
          trim_right_if(path, is_any_of("/"));
          trim_left_if(path, is_any_of("/"));
          //cerr << "path: " << path << endl;
          if (path.size() == 0 && groupName.size() == 0) {
            for (Hash::const_iterator it = dataBlockHash.begin(); it != dataBlockHash.end(); ++it) {
              const string& key = it->first;
              m_recordElementHash.set(key, dataBlockHash.get<RecordElement::Pointer > (key));
            }
          } else if (path.size() == 0) {
            m_recordElementHash.set(groupName, dataBlockHash);
          } else {
            // note: use / instead of . for setFromPath
            m_recordElementHash.setFromPath(path + "/" + groupName, dataBlockHash, "/");
          }
        }
        //cerr << "m_recordElementHash:\n" << m_recordElementHash << endl;

      }

      void RecordFormat::getHash(Hash& recordFormat) {
        //TODO check if root can be already set in buildRecordFormat 
        //cerr << "m_root.size(): " << m_root.size() << endl;
        if (m_root.size() > 0) {
          recordFormat.setFromPath(m_root, m_recordElementHash);
        } else {
          recordFormat = m_recordElementHash;
        }
      }

      Hash RecordFormat::getConfig() {
        return Hash("RecordFormat", m_config);
      }

    }
  }
}
