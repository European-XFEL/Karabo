/*
 * $Id: testDiscoverFormatFromData.cc 5223 2012-02-23 13:35:32Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <exfel/util/Test.hh>
#include "../hdf5/Table.hh"


using namespace std;
using namespace exfel::util;
using namespace exfel::io::hdf5;

int testDiscoverFormatFromData(int argc, char** argv) {

  try {

    Test t;
    TEST_INIT(t, argc, argv);

    cout << t << endl;
    // use t.file("filename"); to access file

    Hash data1, data2, data3, header;
    int c = 12;
    double d = 0.125;
    unsigned short us = 20;

    vector<signed char> vec01(10, 52);
    vector<short> vec02(10, 11);
    vec02[2] = 12;
    vector<int> vec03(10, 11);
    vector<long long> vec04(10, 11);
    vector<unsigned short> vec05(10, 11);
    vector<unsigned int> vec06(10, 11);
    vector<unsigned long long> vec07(10, 11);
    vector<unsigned char> vec08(10, 11);
    vector<double> vec09(10, 11.0);
    vector<float> vec10(10, 11.0);
    vector<string> vec11(10, "ala");
    deque<bool> vec12(10, true);




    data1.setFromPath("a1.db1.c", c);
    data1.setFromPath("a1.db1.d", d);
    data1.setFromPath("a1.db2.f", c);
    data1.setFromPath("a1.db2.g", d);
    data1.setFromPath("a1.db3.us", us);
    data1.setFromPath("a2.db1.c", 2 * c);
    data1.setFromPath("a2.db1.d", 2 * d);
    data1.setFromPath("a1.db5.vec01", vec01);
    data1.setFromPath("a1.db5.vec02", vec02);
    data1.setFromPath("a1.db5.vec03", vec03);
    data1.setFromPath("a1.db5.vec04", vec04);
    data1.setFromPath("a1.db5.vec05", vec05);
    data1.setFromPath("a1.db5.vec06", vec06);
    data1.setFromPath("a1.db5.vec07", vec07);
    data1.setFromPath("a1.db5.vec08", vec08);
    data1.setFromPath("a1.db5.vec09", vec09);
    data1.setFromPath("a1.db5.vec10", vec10);
    data1.setFromPath("a1.db5.vec11", vec11);
    //data1.setFromPath("a1.db5.vec12", vec12);




    header.setFromPath("user.Run", 220);
    header.setFromPath("user.Instrument", "SPB");


    DataFormat::Pointer format = DataFormat::discoverFromData(data1);
    DataFormat::Pointer headerFormat = DataFormat::discoverFromData(header);

    Hash confFile;
    confFile.setFromPath("Hdf5.filename", t.file("discoverFromData1.h5"));
    File::Pointer file = File::create(confFile);

    file->open(File::TRUNCATE);
    Table::Pointer tableHeader = file->createTable("/Header", headerFormat);
    tableHeader->append(header);

    Table::Pointer table = file->createTable("/RawData", format);
    for (int i = 0; i < 5; ++i) {
      table->append(data1);
    }
    file->close();



  } catch (Exception e) {
    cout << e;
    RETHROW
  }

  return 0;
}

