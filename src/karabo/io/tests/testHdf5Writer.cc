/*
 * $Id: testHdf5Writer.cc 5231 2012-02-23 16:26:03Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <boost/shared_array.hpp>
#include <exfel/util/Test.hh>
#include <exfel/util/Time.hh>
#include <exfel/util/Hash.hh>
#include "../hdf5/Table.hh"
#include "../hdf5/File.hh"
#include "../ArrayView.hh"
#include "../Writer.hh"
#include "../Reader.hh"
#include "../hdf5/FLArrayFilter.hh"
#include "../hdf5/DataTypes.hh"
#include "../hdf5/TypeTraits.hh"

using namespace std;
using namespace exfel::util;
using namespace exfel::io;
using namespace exfel::io::hdf5;

namespace testHdf5WriterNS {
    void compute(Hash& rec, int idx);
}

const size_t arraySize = 6;

vector<signed char> va(arraySize);
vector<short> vb(arraySize);
vector<int> vc(arraySize);
vector<long long > vd(arraySize);
vector<unsigned char> ve(arraySize);
vector<unsigned short> vf(arraySize);
vector<unsigned int> vg(arraySize);
vector<unsigned long long> vh(arraySize);
vector<float> vo(arraySize);
vector<double> vp(arraySize);
deque<bool> vx(arraySize);
vector<string> vs(arraySize);

int testHdf5Writer(int argc, char** argv) {

    try {

	Test t;
	TEST_INIT(t, argc, argv);

	cout << t << endl;


	std::cout << "float registry" << std::endl;
	std::cout << GenericFactory< Factory<FLArrayFilter<float> > >::getInstance().getKeysAsString();
	std::cout << "string registry" << std::endl;
	std::cout << GenericFactory< Factory<FLArrayFilter<std::string> > >::getInstance().getKeysAsString();
	std::cout << "bool registry" << std::endl;
	std::cout << GenericFactory < Factory < FLArrayFilter<bool> > >::getInstance().getKeysAsString();
	std::cout << "FLArrayFilterType registry" << std::endl;
	std::cout << GenericFactory < Factory < DataTypes > >::getInstance().getKeysAsString();
	std::cout << "\ntypeid (exfel::io::ArrayView<bool>).name(): " << typeid (exfel::io::ArrayView<bool>).name() << std::endl;
	std::cout << "ArrayTypeTraits::classId<bool > (): " << ArrayTypeTraits::classId<bool >() << std::endl;



	File file(t.file("writer.h5"));
	file.open(File::TRUNCATE);
	size_t vecSize = 6;
	vector<Hash> data(vecSize);

	for (size_t i = 0; i < vecSize; ++i) {
	    testHdf5WriterNS::compute(data[i], i);
	}

	DataFormat::Pointer dataFormat;
	Hash dfc;
	bool discoverConfig = true;
	if (discoverConfig) {
	    try {
		dataFormat = DataFormat::discoverFromData(data[0]);
	    } catch (...) {
		RETHROW;
	    }
	    dfc = dataFormat->getConfig();
	    cout << "dataFormatConfig: " << endl << dfc << endl;
	    Writer<Hash>::Pointer wc = Writer<Hash>::create(Hash("TextFile.filename", "writer.xml"));
	    wc->write(dfc);
	} else {
	    Reader<Hash>::Pointer rc = Reader<Hash>::create(Hash("TextFile.filename", "writerConv.xml"));
	    rc->read(dfc);
	}
	dataFormat = DataFormat::create(dfc);


	cout << "-----" << endl << data[0] << endl << "-----";



	Table::Pointer table = file.createTable("/test", dataFormat);

	cout << "table created " << endl;
	for (size_t i = 0; i < vecSize; ++i) {
	    table->append(data[i]);
	}


	file.close();


    } catch (Exception e) {
	cout << e;
	RETHROW
    }

    return 0;
}

namespace testHdf5WriterNS {

    boost::shared_array<signed char> aaArr(new signed char[arraySize]);
    boost::shared_array<short> abArr(new short[arraySize]);
    boost::shared_array<int> acArr(new int[arraySize]);
    boost::shared_array<long long> adArr(new long long[arraySize]);
    boost::shared_array<unsigned char> aeArr(new unsigned char[arraySize]);
    boost::shared_array<unsigned short> afArr(new unsigned short[arraySize]);
    boost::shared_array<unsigned int> agArr(new unsigned int[arraySize]);
    boost::shared_array<unsigned long long> ahArr(new unsigned long long[arraySize]);
    boost::shared_array<float> aoArr(new float[arraySize]);
    boost::shared_array<double> apArr(new double [arraySize]);
    boost::shared_array<bool> axArr(new bool [arraySize]);
    boost::shared_array<string> asArr(new string [arraySize]);

    void compute(Hash& rec, int idx) {


	signed char a = idx;
	short b = idx;
	int c = idx;
	long long d = idx;
	unsigned char e = idx;
	unsigned short f = idx;
	unsigned int g = idx;
	unsigned long long h = idx;
	float o = idx;
	double p = idx;
	bool x = false;
	if (idx % 2) x = true;
	std::ostringstream str;
	str << "Hello " << idx << " World!!! ";
	string s = str.str();

	//char a = 'a'+ (idx%20);





	for (size_t i = 0; i < arraySize; ++i) {
	    va[i] = i + idx;
	    vb[i] = i + idx;
	    vc[i] = i + idx;
	    vd[i] = i + idx;
	    ve[i] = i + idx;
	    vf[i] = i + idx;
	    vg[i] = i + idx;
	    vh[i] = i + idx;
	    vo[i] = i + idx;
	    vp[i] = i + idx;
	   // vx[i] = false;
	   // if (i % 2) vx[i] = true;
	    std::ostringstream str;
	    str << "Hello " << idx << "[" << i << "]" << " from me";
	    vs[i] = str.str();
	}







	rec.setFromPath("scalars.a", a);
	rec.setFromPath("scalars.b", b);
	rec.setFromPath("scalars.c", c);
	rec.setFromPath("scalars.d", d);
	rec.setFromPath("scalars.e", e);
	rec.setFromPath("scalars.f", f);
	rec.setFromPath("scalars.g", g);
	rec.setFromPath("scalars.h", h);
	rec.setFromPath("scalars.o", o);
	rec.setFromPath("scalars.p", p);
	rec.setFromPath("scalars.x", x);
	rec.setFromPath("scalars.s", s);

	//ArrayView<int> avc(acArr.get(), 2, 3);


	ArrayView<signed char> vaView(va);
	rec.setFromPath("arrayView.va", vaView);
	rec.setFromPath("arrayView.vb", ArrayView<short>(vb));
	rec.setFromPath("arrayView.vc", ArrayView<int>(vc));
	rec.setFromPath("arrayView.vd", ArrayView<long long>(vd));
	rec.setFromPath("arrayView.ve", ArrayView<unsigned char>(ve));
	rec.setFromPath("arrayView.vf", ArrayView<unsigned short>(vf));
	rec.setFromPath("arrayView.vg", ArrayView<unsigned int>(vg));
	rec.setFromPath("arrayView.vh", ArrayView<unsigned long long>(vh));
	rec.setFromPath("arrayView.vo", ArrayView<float>(vo));
	rec.setFromPath("arrayView.vp", ArrayView<double>(vp));
	rec.setFromPath("arrayView.vs", ArrayView<std::string > (vs));
	//rec.setFromPath("arrayView.vx", ArrayView<bool>(axArr.get(), arraySize));


	rec.setFromPath("vectors.va", va);
	rec.setFromPath("vectors.vb", vb);
	rec.setFromPath("vectors.vc", vc);
	rec.setFromPath("vectors.vd", vd);
	rec.setFromPath("vectors.ve", ve);
	rec.setFromPath("vectors.vf", vf);
	rec.setFromPath("vectors.vg", vg);
	rec.setFromPath("vectors.vh", vh);
	rec.setFromPath("vectors.vo", vo);
	rec.setFromPath("vectors.vp", vp);
	rec.setFromPath("vectors.vs", vs);
	//rec.setFromPath("deque.vx", vx);

    }
}
