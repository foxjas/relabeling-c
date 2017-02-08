#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

using namespace std;

char* getOption(const char* option, int argc, char **argv) {
  for (int i = 1; i < argc-1; i++) {
      if (strcmp(argv[i], option) == 0)
          return argv[i+1];
  }
  return NULL;
}

bool hasOption(const char* option, int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
      if (strcmp(argv[i], option) == 0)
          return true;
  }
  return false;
}

// Driver function to sort the vector elements by second element of pairs, ascending order
bool sortBySecAsec(const std::pair<int,int> &a, const std::pair<int,int> &b) {
    return (a.second < b.second);
}

// Driver function to sort the vector elements by second element of pairs, descending order
bool sortBySecDesc(const std::pair<int,int> &a, const std::pair<int,int> &b) {
    return (a.second > b.second);
}