/*
 * $Id: RecordElement.cc 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "RecordElement.hh"
#include "FixedLengthArray.hh"
#include <boost/shared_array.hpp>
#include "../iodll.hh"


using namespace std;
using namespace karabo::util;
using namespace H5;
using namespace boost;

namespace karabo {
    namespace io {

	void RecordElement::expectedParameters(Schema& expected) {

	    STRING_ELEMENT(expected)
		    .key("dataset")
		    .displayedName("DataSet name")
		    .description("Data set name. i.e.: d1, g4.d2")
		    .assignmentMandatory()
		    .reconfigurable()
		    .commit();

	    // TODO: not supported at the moment - reconsider
	    BOOL_ELEMENT(expected)
		    .key("implicitConversion")
		    .displayedName("Implicit Conversion")
		    .description("Allow Implicit Conversion")
		    .assignmentOptional().defaultValue(false)
		    .reconfigurable()
		    .commit();

	    INT32_ELEMENT(expected)
		    .key("compressionLevel")
		    .displayedName("Use Compression Level")
		    .description("Defines compression level: [0-9]. 0 - no compression, 9 - attempt the best compression")
		    .minInc(0).maxInc(9)
		    .assignmentOptional().defaultValue(0)
		    .reconfigurable()
		    .commit();

	}

	void RecordElement::configure(const Hash& input) {

	    string fullName = input.get<string > ("dataset");
	    m_compressionLevel = input.get<int>("compressionLevel");
	    m_implicitConversion = input.get<bool>("implicitConversion");

	    boost::trim(fullName);
	    vector<string> v;
	    boost::split(v, fullName, is_any_of("."));

	    if (v.size() == 0) {
		throw KARABO_PARAMETER_EXCEPTION("Dataset name cannot be an empty string");
	    }
	    m_key = v[v.size() - 1];

	    for (size_t i = 0; i < v.size() - 1; ++i) {
		m_relativeGroup += v[i];
		if (i < v.size() - 2) m_relativeGroup += "/";
	    }
	}

	const string& RecordElement::getName() {
	    return m_key;
	}

	void RecordElement::getElement(Hash& element) {
	    if (m_relativeGroup.size() > 0) {
		element.setFromPath(m_relativeGroup, Hash());
		Hash& relativeGroupHash = element.getFromPath<Hash > (m_relativeGroup);
		relativeGroupHash.set(m_key, shared_from_this());
	    } else {
		element.set(m_key, shared_from_this());
	    }
	}

	void RecordElement::extend(hsize_t size) {
	    hsize_t currentSizeDims[1];
	    m_fileDataSpace.getSimpleExtentDims(currentSizeDims);
	    hsize_t newSizeDims[1];
	    newSizeDims[0] = currentSizeDims[0] + size;
	    m_dataSet.extend(newSizeDims);
	    m_fileDataSpace = m_dataSet.getSpace();
	}

	void RecordElement::open(boost::shared_ptr<Group> group) {
	    m_group = group;
	    try {
		m_dataSet = group->openDataSet(m_key.c_str());
		m_fileDataSpace = m_dataSet.getSpace();
	    } catch (...) {
		KARABO_RETHROW
	    }
	}

	hsize_t RecordElement::getNumberOfRecords() {
	    int ndims = m_fileDataSpace.getSimpleExtentNdims();
	    boost::shared_array<hsize_t> currentSizeDims(new hsize_t[ndims]);
	    m_fileDataSpace.getSimpleExtentDims(currentSizeDims.get());
	    return currentSizeDims[0];
	}

	hsize_t RecordElement::getChunkSize() {

	   // TODO this is not generic enough
	    DSetCreatPropList cparms = m_dataSet.getCreatePlist();
	    hsize_t chunk_dims[1];
	    chunk_dims[0] = 0;
	    int rank_chunk;
	    if (H5D_CHUNKED == cparms.getLayout()) {
		rank_chunk = cparms.getChunk(1, chunk_dims);
		tracer << "chunk rank " << rank_chunk << " dimensions "
			<< (unsigned long) (chunk_dims[0]) << endl;
	    }
	    return chunk_dims[0];

	}

	DataSpace RecordElement::scalarFileDataSpace(hsize_t size) {
	    hsize_t dims[1];
	    dims[0] = size;
	    hsize_t maxdims[] = {H5S_UNLIMITED};
	    return DataSpace(1, dims, maxdims);
	}

	DataSpace RecordElement::scalarDataSpace() {
	    return DataSpace(H5S_SCALAR);
	}

	void RecordElement::createDataSetProperties(hsize_t chunkSize) {
	    m_dataSetProperties = boost::shared_ptr<DSetCreatPropList > (new DSetCreatPropList());
	    if (m_compressionLevel > 0) {
		//         m_dataSetProperties->setShuffle();
		m_dataSetProperties->setDeflate(m_compressionLevel);
	    }
	    hsize_t chunkDims[1] = {chunkSize};
	    m_dataSetProperties->setChunk(1, chunkDims);
	}

	void RecordElement::readAttributes(karabo::util::Hash& attributes) {
	    attributes.setFromPath(m_key + ".type", this->getClassInfo().getClassId());
	    readSpecificAttributes(attributes);
	}

	void RecordElement::readSpecificAttributes(karabo::util::Hash& attributes) {
	}

	void RecordElement::selectFileRecord(hsize_t recordId, hsize_t len) {
	    hsize_t start[1];
	    hsize_t count[1];
	    start[0] = recordId;
	    count[0] = len;
	    m_fileDataSpace.selectHyperslab(H5S_SELECT_SET, count, start, NULL, NULL);
	}




    }
}
