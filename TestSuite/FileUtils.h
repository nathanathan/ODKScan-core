#include <string>
#include <vector>

// crawls a directory rootdir for filenames and appends them to the filenames
// vector parameter (modifies filenames vector)
int CrawlFileTree(std::string rootdir, std::vector<std::string> &filenames);
int CrawlFileTree(char* rootdir, std::vector<std::string > &filenames);
