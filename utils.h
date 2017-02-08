#include <utility>

char* getOption(const char* option, int argc, char **argv);

bool hasOption(const char* option, int argc, char **argv);

// Driver function to sort the vector elements by second element of pairs, ascending order
bool sortBySecAsec(const std::pair<int,int> &a, const std::pair<int,int> &b);

// Driver function to sort the vector elements by second element of pairs, descending order
bool sortBySecDesc(const std::pair<int,int> &a, const std::pair<int,int> &b);