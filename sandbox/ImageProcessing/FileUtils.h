#include <string>
#include <iostream>
#include <fstream>
#include <vector>

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
}

// crawls a directory rootdir for filenames and appends them to the filenames
// vector parameter (modifies filenames vector)
//
// to call with a C++ string, call like so:
//   string str("directory_name");
//   cstr = new char[str.size() + 1];
//   strcpy(cstr, str.c_str());
//
//   CrawlFileTree(cstr, filenames);
//
//   delete[] cstr;
//
// required libraries:
//   #include <cstring>
//   #include <string>
int CrawlFileTree(char *rootdir, std::vector<std::string> &filenames);
