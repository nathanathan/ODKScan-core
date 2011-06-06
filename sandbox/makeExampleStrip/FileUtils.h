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
//   CrawlFileTree(directory_string.c_str(), filenames);
int CrawlFileTree(const char *rootdir, vector<string> &filenames);
