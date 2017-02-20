#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <unordered_map>
#include <assert.h>
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

bool sortByPairAsec(const std::pair<int,int> &a, const std::pair<int,int> &b) {
    if (a.first < b.first) {
      return true;
    } else if (a.first > b.first) {
      return false;
    } else {
      return (a.second < b.second);
    }
}

bool sortByPairDesc(const std::pair<int,int> &a, const std::pair<int,int> &b) {
    if (a.first > b.first) {
      return true;
    } else if (a.first < b.first) {
      return false;
    } else {
      return (a.second > b.second);
    }
}

void writeDegreesFile(char* inputGraphPath, char* outputPath) {
    vertexId_t nv;
    length_t ne;
    nv = ne = -1;
    const int MAX_CHARS = 1000;
    char temp[MAX_CHARS];
    char *written;
    FILE *fp = fopen(inputGraphPath, "r");

    string inFileName(inputGraphPath);
    bool isSnapInput = inFileName.find(".txt")==std::string::npos?false:true;
    bool isMarketInput = inFileName.find(".mtx")==std::string::npos?false:true;

    if (isSnapInput) {
        written = fgets(temp, MAX_CHARS, fp);
        while ((nv == -1 || ne == -1) && written != NULL) {
            sscanf(temp, "# Nodes: %d Edges: %d\n", &nv,&ne);
            written = fgets(temp, MAX_CHARS, fp);
        }
        if ((nv == -1 || ne == -1) && written == NULL) {
            fprintf(stderr, "SNAP input file is missing header info\n");
            exit(-1);
        }
        while (written != NULL && *temp == '#') { // skip any other comments
            written = fgets(temp, MAX_CHARS, fp);
        }
    } else if (isMarketInput) {
        while (fgets(temp, MAX_CHARS, fp) && *temp == '%'); // skip comments
        sscanf(temp, "%d %*s %d\n", &nv,&ne); // read Matrix Market header
        fgets(temp, MAX_CHARS, fp);
    } else {
        fprintf(stderr, "Could not recognize input file format\n");
        exit(-1);
    }

    length_t counter = 0;
    vector<int> degrees(nv, 0);
    vertexId_t srctemp,desttemp;

    // read in edges
    while(counter<ne)
    {
        sscanf(temp, "%d %d\n", (vertexId_t*)&srctemp, (vertexId_t*)&desttemp);
        if (isMarketInput) {
            srctemp -= 1;
            desttemp -= 1;
        }
        degrees[srctemp]++;
        degrees[desttemp]++;
        counter++;
        fgets(temp, MAX_CHARS, fp);
    }
    fclose (fp);

    // count degrees
    vector<pair<vertexId_t, int> > vid_degree_pairs(nv);
    for (int i=0; i<nv; i++) {
        vid_degree_pairs[i] = make_pair(i, degrees[i]);
    }

    sort(vid_degree_pairs.begin(), vid_degree_pairs.end(), sortBySecDesc);

    // write out degrees file
    ofstream fout;
    fout.open(outputPath);
    for (vertexId_t i=0; i<nv; i++) {
        fout << vid_degree_pairs[i].first << " " << vid_degree_pairs[i].second << "\n";
    }
    fout.close();
}

void writeCommunityStats(char *inputPath, char *outputPath) {
    int nComs=0;
    double avgComSize=0;
    unordered_map<int,int> com_to_size;
    vertexId_t a_prev = -1;
    vertexId_t a;
    int b;
    char *written;
    const int MAX_CHARS = 100;
    char temp[MAX_CHARS];
    
    FILE *fp = fopen(inputPath, "r");
    // read in (vertex id, community id) pairs
    written = fgets(temp, MAX_CHARS, fp);
    sscanf(temp, "%d %d\n", (vertexId_t*)&a, (vertexId_t*)&b);
    while((written != NULL) && a > a_prev) {
        com_to_size[b]+=1;
        a_prev = a;
        written = fgets(temp, MAX_CHARS, fp);
        sscanf(temp, "%d %d\n", (vertexId_t*)&a, (vertexId_t*)&b);
    }
    fclose (fp);

    nComs = com_to_size.size();
    map<int,int> size_to_count;
    int comSize;
    for (unordered_map<int,int>::iterator element = com_to_size.begin(); element != com_to_size.end(); element++) {
        comSize = element->second;
        size_to_count[comSize] += 1;
        avgComSize += comSize;
    }
    avgComSize /= nComs;

    ofstream fout;
    fout.open(outputPath);
    fout << "# Number of communities: " << nComs << endl;
    fout << "# Average community size: " << avgComSize << endl;
    for (map<int,int>::iterator element = size_to_count.begin(); element != size_to_count.end(); element++) {
        fout << element->first << " " << element->second << "\n";
    }
    fout.close();
}

int checkDuplicateEdges(char* inputGraphPath) {
    vertexId_t nv;
    length_t   ne;
    nv = ne = -1;
    const int MAX_CHARS = 1000;
    char temp[MAX_CHARS];
    char *written;
    FILE *fp = fopen(inputGraphPath, "r");

    string inFileName(inputGraphPath);
    bool isSnapInput = inFileName.find(".txt")==std::string::npos?false:true;
    bool isMarketInput = inFileName.find(".mtx")==std::string::npos?false:true;

    if (isSnapInput) {
        written = fgets(temp, MAX_CHARS, fp);
        while ((nv == -1 || ne == -1) && written != NULL) {
            sscanf(temp, "# Nodes: %d Edges: %d\n", &nv,&ne);
            written = fgets(temp, MAX_CHARS, fp);
        }
        if ((nv == -1 || ne == -1) && written == NULL) {
            fprintf(stderr, "SNAP input file is missing header info\n");
            exit(-1);
        }
        while (written != NULL && *temp == '#') { // skip any other comments
            written = fgets(temp, MAX_CHARS, fp);
        }
    } else if (isMarketInput) {
        while (fgets(temp, MAX_CHARS, fp) && *temp == '%'); // skip comments
        sscanf(temp, "%d %*s %d\n", &nv,&ne); // read Matrix Market header
        fgets(temp, MAX_CHARS, fp);
    } else {
        fprintf(stderr, "Could not recognize input file format\n");
        exit(-1);
    }

    vector<pair<int, int> > edges(ne);
    vertexId_t counter=0;
    vertexId_t srctemp,desttemp;

    // read in edges
    while(counter<ne)
    {
        sscanf(temp, "%d %d\n", (vertexId_t*)&srctemp, (vertexId_t*)&desttemp);
        edges[counter] = make_pair(min(srctemp, desttemp), max(srctemp, desttemp));
        counter++;
        fgets(temp, MAX_CHARS, fp);
    }
    fclose (fp);
    assert(counter == ne);

    sort(edges.begin(), edges.end(), sortByPairAsec);
    
    int prev_first = -1;
    int prev_second = -1;
    int first, second;
    int duplicates = 0;
    for (vector<pair<vertexId_t, int> >::iterator pair = edges.begin(); pair != edges.end(); pair++) {
        first = pair->first;
        second = pair->second;
        if (first == prev_first && second == prev_second)
            duplicates += 1;
        prev_first = first;
        prev_second = second;
    }
    return duplicates;
}