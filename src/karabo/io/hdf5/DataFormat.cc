/*
 * $Id: DataFormat.cc 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "DataFormat.hh"
#include "Scalar.hh"
#include "FixedLengthArray.hh"
#include "../ArrayView.hh"
#include "DataTypes.hh"


using namespace std;
using namespace karabo::util;

namespace karabo {
  namespace io {
    namespace hdf5 {

      KARABO_REGISTER_ONLY_ME_CC(DataFormat)

      void DataFormat::expectedParameters(Schema& expected) {

        SINGLE_ELEMENT<RecordFormat > (expected)
                .key("RecordFormat")
                .displayedName("Record Format")
                .description("Definition of record format.")
                .assignmentMandatory()
                .reconfigurable()
                .commit();


        NON_EMPTY_LIST_ELEMENT<DataBlock > (expected)
                .key("dataBlocks")
                .displayedName("Data Blocks")
                .description("Definition of available Data Blocks. Each Data Block which should be written to a file must be explicitly included in the RecordFormat.")
                .assignmentMandatory()
                .reconfigurable()
                .commit();


      }

      void DataFormat::configure(const Hash& input) {
        m_config = Hash("DataFormat", input);        
      }

      const RecordFormat::Pointer DataFormat::getRecordFormat() {
        const Hash& input = m_config.get<Hash > ("DataFormat");
        RecordFormat::Pointer recordFormat = RecordFormat::createSingle("RecordFormat", "RecordFormat", input);
        std::vector<DataBlock::Pointer > dataBlockList = DataBlock::createList("dataBlocks", input);
        recordFormat->buildRecordFormat(dataBlockList);
        return recordFormat;
      }

      const Hash& DataFormat::getConfig() const {
        return m_config;
      }

      DataFormat::Pointer DataFormat::discoverFromData(const Hash& data) {
        // TODO: what to do if it results in two data blocks having the same name but different definition
        // i.e.:
        // /a/b/c/d/x
        // /a/b/c/d/y
        // /a/d/z



        Hash dataBlocks, recordFormat, dataFormatConfig;
        Hash flat = data.flatten("/");

        //cerr << "Flatten Hash: \n" << flat << endl;

        vector<string> tokens;
        map<string, int> idxBlock;
        for (Hash::const_iterator it = flat.begin(); it != flat.end(); ++it) {

          string key = it->first;

          boost::any anyValue = flat.getAny(it);

          ArrayDimensions arraySize;
          string type = "";
          DataTypes::Pointer dataType;
          try {
            dataType = DataTypes::createDefault(anyValue.type().name());
            type = dataType->getElementClassId();            
            arraySize = dataType->getDims(anyValue);
          } catch (...) {
            throw KARABO_HDF_IO_EXCEPTION("Not supported container/value type rtti["+string(anyValue.type().name())+"]");
          }

          bool isArray = (arraySize[0] > 0 ? true : false);
          // not efficient for large paths (requires copying strings to vector), but we do not expect them here
          boost::split(tokens, key, boost::is_any_of("/"));
          //cerr << "tokens.size(): " << tokens.size() << endl;
          int dsPosition = tokens.size() - 1;
          string datasetName = tokens[dsPosition];
          int dbPosition = dsPosition - 1;
          string dataBlockName;
          if (dbPosition < 0) {
            dataBlockName = ""; // special case where dataset belongs directly to the root group -> root data block
          } else {
            dataBlockName = tokens[dbPosition];
          }

          ostringstream pathStream;
          for (int i = 0; i < dbPosition; ++i) {
            pathStream << "/" << tokens[i];
          }
          string path = pathStream.str();
          //cerr << "path: " << path << endl;


          if (!dataBlocks.has(dataBlockName)) {
            dataBlocks.set(dataBlockName, Hash());
            idxBlock[dataBlockName] = 0;
          }

          if (!recordFormat.has(path + dataBlockName)) {
            recordFormat.setFromPath(path + dataBlockName + ".name", dataBlockName);
            recordFormat.setFromPath(path + dataBlockName + ".path", path);
          }



          ostringstream os;
          os << "DataBlock/elements[" << idxBlock[dataBlockName] << "]/" << type << "/dataset";
          Hash& flatFormat = dataBlocks.get<Hash > (dataBlockName);
          flatFormat.set(os.str(), datasetName);
          if (isArray) {
            ostringstream os2;
            os2 << "DataBlock/elements[" << idxBlock[dataBlockName] << "]/" << type << "/dims";
            flatFormat.set(os2.str(), arraySize.toVector());

            //cout << "os2: " << os2.str() << endl;
          }
          idxBlock[dataBlockName] = idxBlock[dataBlockName]++;
        }

        //cerr << "record: \n" << recordFormat << endl;

        vector<Hash> dataBlockVector;
        for (Hash::const_iterator it = dataBlocks.begin(); it != dataBlocks.end(); ++it) {
          string key = it->first;
          Hash flatFormat = dataBlocks.get<Hash > (key);
          flatFormat.set("DataBlock/name", key);
          dataBlockVector.push_back(flatFormat.unflatten("/"));
        }

        vector<Hash> recordFormatVector;
        for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
          string key = it->first;
          Hash record;
          record.setFromPath("Group.name", recordFormat.getFromPath<string > (key + ".name"));
          record.setFromPath("Group.path", recordFormat.getFromPath<string > (key + ".path"));
          recordFormatVector.push_back(record);
        }
        dataFormatConfig.setFromPath("DataFormat.dataBlocks", dataBlockVector);
        dataFormatConfig.setFromPath("DataFormat.RecordFormat.groups", recordFormatVector);

        return DataFormat::create(dataFormatConfig);

      }

      string DataFormat::getClassIdAsString(const boost::any& any) {
        try {
          //cout << "any: type = " << any.type().name() << endl;
          DataTypes::Pointer discriminator = DataTypes::createDefault(any.type().name());
          return discriminator->getElementClassId();
        } catch (...) {
          throw KARABO_HDF_IO_EXCEPTION("Not supported container/value type");
        }

      }

      ArrayDimensions DataFormat::arraySize(const boost::any& anyValue) {

        ArrayDimensions dims(1, 0);
        try {
          DataTypes::Pointer discriminator = DataTypes::createDefault(anyValue.type().name());
          dims = discriminator->getDims(anyValue);
          return dims;
        } catch (...) {
          throw KARABO_HDF_IO_EXCEPTION("Not supported container/value type");
        }

      }


    }
  }
}
