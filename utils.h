#include <utility>

typedef int32_t vertexId_t;
typedef vertexId_t length_t;

char* getOption(const char* option, int argc, char **argv);

int getIntOption(const char* option, int argc, char **argv);

bool hasOption(const char* option, int argc, char **argv);

// Driver function to sort the vector elements by second element of pairs, ascending order
bool sortBySecAsec(const std::pair<int,int> &a, const std::pair<int,int> &b);

// Driver function to sort the vector elements by second element of pairs, descending order
bool sortBySecDesc(const std::pair<int,int> &a, const std::pair<int,int> &b);

bool sortByPairAsec(const std::pair<int,int> &a, const std::pair<int,int> &b);

bool sortByPairDesc(const std::pair<int,int> &a, const std::pair<int,int> &b);

void writeDegreesFile(char* inputGraphPath, char* outputPath);

void writeCommunityStats(char *inputPath, char *outputPath);

int checkDuplicateEdges(char* inputGraphPath);