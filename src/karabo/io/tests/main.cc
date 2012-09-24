/*
 * $Id
 *
 * File:   main.cc
 * Author: <kerstin.weger@xfel.eu>
 *
 * Created on March 19, 2012, 10:57 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#if defined(_WIN32)

#include <iostream>
#include <string>

//#include "testHdf5Writer.cc"
//#include "testHdf5Reader.cc"
#include "testReader.cc"
//#include "testRecordElement.cc"
//#include "testSim.cc"

using namespace exfel::util;
using namespace exfel::io;

/* Create map.  */

typedef int (*MainFuncPointer)(int , char*[]);
typedef struct
{
  const char* name;
  MainFuncPointer func;
} functionMapEntry;

functionMapEntry cmakeGeneratedFunctionMapEntries[] = {
    //{
    //"testRecordElement",
    //testRecordElement
    //},
  //{
  //  "testHdf5Reader",
  //  testHdf5Reader
  //},
  //{
  //  "testHdf5Writer",
  //  testHdf5Writer
  //},
  {
    "testReader",
    testReader
  },
  //{
  //  "testSim",
  //  testSim
  //},
  //{
  //  "testFormat",
  //  testFormat
  //},
  //{
  //  "testDiscoverFormatFromData",
  //  testDiscoverFormatFromData
  //},
  //{
  //  "testArray",
  //  testArray
  //},

  {0,0}
};

/* Allocate and create a lowercased copy of string
   (note that it has to be free'd manually) */

char* lowercase(const char *string)
{
  char *new_string, *p;

#ifdef __cplusplus
  new_string = static_cast<char *>(malloc(sizeof(char) *
    static_cast<size_t>(strlen(string) + 1)));
#else
  new_string = (char *)(malloc(sizeof(char) * (size_t)(strlen(string) + 1)));
#endif

  if (!new_string)
    {
    return 0;
    }
  strcpy(new_string, string);
  p = new_string;
  while (*p != 0)
    {
#ifdef __cplusplus
    *p = static_cast<char>(tolower(*p));
#else
    *p = (char)(tolower(*p));
#endif

    ++p;
    }
  return new_string;
}

int main(int argc, char** argv) {

	//try {

 //   Hash config("TextFile.filename", "test.xml");

 //   Hash content("This.is.a.test", "sure");

 //   std::cout << "Registered: " << String::sequenceToString(Factory<Writer<Hash> >::getRegisteredKeys()) << std::endl;

 //   Writer<Hash>::Pointer out = Writer<Hash>::create(config);

 //   out->write(content);
 //   } catch (const Exception& e) {
 //       std::cout << e << std::endl;
 //   }

    int i, NumTests, testNum, partial_match;
    char *arg, *test_name;
    int count;
    int testToRun = -1;
    
    for(count =0; cmakeGeneratedFunctionMapEntries[count].name != 0; count++) {
    }
    NumTests = count;
    /* If no test name was given */
    /* process command line with user function.  */
    if (argc < 2) {
        /* Ask for a test.  */
        printf("Available tests:\n");
        for (i =0; i < NumTests; ++i) {
            printf("%3d. %s\n", i, cmakeGeneratedFunctionMapEntries[i].name);
        }
        printf("To run a test, enter the test number: ");
        fflush(stdout);
        testNum = 0;
        if( scanf("%d", &testNum) != 1 ) {
            printf("Couldn't parse that input as a number\n");
            return -1;
        }
        if (testNum >= NumTests) {
            printf("%3d is an invalid test number.\n", testNum);
            return -1;
        }
        testToRun = testNum;
        argc--;
        argv++;
    }
    partial_match = 0;
    arg = 0;
    /* If partial match is requested.  */
    if(testToRun == -1 && argc > 1) {
        partial_match = (strcmp(argv[1], "-R") == 0) ? 1 : 0;
    }
    if (partial_match && argc < 3) {
        printf("-R needs an additional parameter.\n");
        return -1;
    }
    if(testToRun == -1) {
        arg = lowercase(argv[1 + partial_match]);
    }
    for (i =0; i < NumTests && testToRun == -1; ++i) {
        test_name = lowercase(cmakeGeneratedFunctionMapEntries[i].name);
        if (partial_match && strstr(test_name, arg) != NULL)
        {
            testToRun = i;
            argc -=2;
            argv += 2;
        }
        else if (!partial_match && strcmp(test_name, arg) == 0)
        {
            testToRun = i;
            argc--;
            argv++;
        }
        free(test_name);
    }
    if(arg) {
        free(arg);
    }
    if(testToRun != -1) {
        //int result;
        (*cmakeGeneratedFunctionMapEntries[testToRun].func)(argc, argv);
        //return result;
    }  
  
    /* Nothing was run, display the test names.  */
    //printf("Available tests:\n");
    //for (i =0; i < NumTests; ++i) {
    //    printf("%3d. %s\n", i, cmakeGeneratedFunctionMapEntries[i].name);
    //}
    //printf("Failed: %s is an invalid test name.\n", argv[1]);

    std::string s;
    std::cin >> s;

	return EXIT_SUCCESS;				
}
#endif
