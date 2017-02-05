#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

typedef int32_t vertexId_t;
typedef vertexId_t length_t;

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
bool sortBySecAsec(const pair<int,int> &a, const pair<int,int> &b) {
    return (a.second < b.second);
}

// Driver function to sort the vector elements by second element of pairs, descending order
bool sortBySecDesc(const pair<int,int> &a, const pair<int,int> &b) {
    return (a.second > b.second);
}

void readGraphSNAP(char* inputGraphPath, char* relabeledGraphPath, char* mappingPath) {
    vertexId_t nv,*src,*dest;
    length_t   ne;
    nv = ne = -1;
    const int MAX_CHARS = 1000;
    char temp[MAX_CHARS];
    char *written;
    FILE *fp = fopen(inputGraphPath, "r");

    // scan for SNAP header comment
    while (nv == -1 || ne == -1) {
    	fgets(temp, MAX_CHARS, fp);
    	sscanf(temp, "# Nodes: %d Edges: %d\n", &nv,&ne);
    }

    written = fgets(temp, MAX_CHARS, fp);
    while (written != NULL && *temp == '#') { // skip any other comments
        written = fgets(temp, MAX_CHARS, fp);
    }

    src = (vertexId_t *) malloc ((ne ) * sizeof (vertexId_t));
    dest = (vertexId_t *) malloc ((ne ) * sizeof (vertexId_t));
    unordered_set<vertexId_t> vertex_set;
    vertexId_t counter=0;
    vertexId_t srctemp,desttemp;

    // read in edges
    while(counter<ne)
    {
        sscanf(temp, "%d %d\n", (vertexId_t*)&srctemp, (vertexId_t*)&desttemp);
        src[counter]=srctemp;
        dest[counter]=desttemp;
        vertex_set.insert(srctemp);
        vertex_set.insert(desttemp);
        counter++;
        fgets(temp, MAX_CHARS, fp);
    }
    fclose (fp);

    // get graph vertices, sorted
    vector<vertexId_t> vertices(vertex_set.begin(), vertex_set.end());
    sort(vertices.begin(), vertices.end());

    // create relabeling map
    unordered_map<vertexId_t, vertexId_t> relabel_map;
    for (length_t i=0; i<nv; i++) {
    	relabel_map[vertices[i]] = i;
    }

    // write out relabeled graph to file
    ofstream fout;
    fout.open(relabeledGraphPath);
    string outFileName(relabeledGraphPath);
    bool isSnap = outFileName.find(".txt")==std::string::npos?false:true;
    bool isMarket = outFileName.find(".mtx")==std::string::npos?false:true;
    if (isSnap) {
        printf("Outputting new SNAP graph\n");
        fout << "# Nodes: " << vertices.size() << " " << "Edges: " << ne << "\n";
    } else if (isMarket) {
        printf("Outputting new matrix market graph\n");
        fout << vertices.size() << " " <<  vertices.size() << " " << ne << "\n";
    } else {
        cout << "Unrecognized output graph file type: defaulting to SNAP .txt" << endl;
        printf("Outputting new SNAP graph\n");
        fout << "# Nodes: " << vertices.size() << " " << "Edges: " << ne << "\n";
    }

    vertexId_t relabeledSrc, relabeledDest;
    for (vertexId_t i=0; i<counter; i++) {
        relabeledSrc = relabel_map[src[i]];
        relabeledDest = relabel_map[dest[i]];
        if (isSnap) {
            fout << relabeledSrc << " " << relabeledDest << "\n";
        } else {
            fout << relabeledSrc+1 << " " << relabeledDest+1 << "\n";
        }
    }
    fout.close();

    if (mappingPath != NULL) {
        fout.open(mappingPath);
        fout << "New ID Old ID\n";
        for (vertexId_t i=0; i<vertices.size(); i++)
            fout << i << " " << vertices[i] << "\n";
    }

    printf("Processed %lu vertices, %d edges as input\n", vertices.size(), counter);
    free(src);
    free(dest);
}

vector<vertexId_t> relabelVerticesFromInfomap(FILE *com_fp, int nv) {
    // read in community-partitioned vertices
    const int MAX_CHARS = 1000;
    char temp[MAX_CHARS];
    vertexId_t vid, i=0;
    vector<vertexId_t> vertices(nv);
    char* written = fgets(temp, MAX_CHARS, com_fp);
    while (written != NULL && *temp == '#') { // skip comments
        written = fgets(temp, MAX_CHARS, com_fp);
    }
    while (written != NULL) { // read vertices in-order
        sscanf(temp, "%d %*s\n", (vertexId_t*)&vid);
        vertices[i] = vid;
        written = fgets(temp, MAX_CHARS, com_fp);
        i += 1;
    }
    return vertices;
}

vector<vertexId_t> relabelVerticesFromLouvain(FILE *com_fp, int nv) {
    vector<pair<vertexId_t, int>> vertex_labels;
    vertex_labels.reserve(nv);
    vertexId_t a_prev = -1;
    vertexId_t a;
    int b;
    char *written;
    const int MAX_CHARS = 100;
    char temp[MAX_CHARS];
    
    // read in initial (vertex id, community id) pairs
    written = fgets(temp, MAX_CHARS, com_fp);
    sscanf(temp, "%d %d\n", (vertexId_t*)&a, (vertexId_t*)&b);
    while((written != NULL) && a > a_prev) {
        vertex_labels.push_back(make_pair(a, b));
        a_prev = a;
        written = fgets(temp, MAX_CHARS, com_fp);
        sscanf(temp, "%d %d\n", (vertexId_t*)&a, (vertexId_t*)&b);
    }
    assert(vertex_labels.size() == nv);
    sort(vertex_labels.begin(), vertex_labels.end(), sortBySecAsec);

    // read in (community id, community id) pairs, and reorder vertices accordingly
    vector<int> com_map;
    while (written != NULL) { // loop condition over entire file
        while ((written != NULL) && a>a_prev) { // iterating over a level
            com_map.push_back(b);
            a_prev = a;
            written = fgets(temp, MAX_CHARS, com_fp);
            sscanf(temp, "%d %d\n", (vertexId_t*)&a, (vertexId_t*)&b);
        }
        if (com_map.size()) { // condition to avoid reordering the 1st time we enter outer while loop
            for (int i=0; i<vertex_labels.size(); i++) {
                vertex_labels[i].second = com_map[vertex_labels[i].second];
            }
            stable_sort(vertex_labels.begin(), vertex_labels.end(), sortBySecAsec);
            com_map.clear();
        }
        a_prev = -1;
    } /** NOTE: ends up calculating & sorting one "useless" identity map at the last level */

    vector<vertexId_t> final;
    final.reserve(nv);
    for (vector<pair<vertexId_t, int>>::iterator pair = vertex_labels.begin(); pair != vertex_labels.end(); pair++) {
        final.push_back((*pair).first);
    }
    assert(final.size() == nv);
    return final;
}

void relabelGraphSNAP(char* inputGraphPath, char* relabeledGraphPath, char* partitionInfoPath, char* mappingPath) {
    vertexId_t nv,*src,*dest;
    length_t   ne;
    nv = ne = -1;
    const int MAX_CHARS = 1000;
    char temp[MAX_CHARS];
    char *written;
    FILE *fp = fopen(inputGraphPath, "r");

    // scan for SNAP header comment
    while (nv == -1 || ne == -1) {
        fgets(temp, MAX_CHARS, fp);
        sscanf(temp, "# Nodes: %d Edges: %d\n", &nv,&ne);
    }

    written = fgets(temp, MAX_CHARS, fp);
    while (written != NULL && *temp == '#') { // skip any other comments
        written = fgets(temp, MAX_CHARS, fp);
    }

    src = (vertexId_t *) malloc ((ne ) * sizeof (vertexId_t));
    dest = (vertexId_t *) malloc ((ne ) * sizeof (vertexId_t));
    vertexId_t counter=0;
    vertexId_t srctemp,desttemp;

    // read in edges
    while(counter<ne)
    {
        sscanf(temp, "%d %d\n", (vertexId_t*)&srctemp, (vertexId_t*)&desttemp);
        src[counter]=srctemp;
        dest[counter]=desttemp;
        counter++;
        fgets(temp, MAX_CHARS, fp);
    }
    fclose (fp);

    vector<vertexId_t> vertices;
    fp = fopen(partitionInfoPath, "r");
    string comFileName(partitionInfoPath);
    bool isInfomap = comFileName.find(".clu")==std::string::npos?false:true;
    bool isLouvain = comFileName.find(".tree")==std::string::npos?false:true;
    // read in community-partitioned vertices
    if (isInfomap) {
        vertices = relabelVerticesFromInfomap(fp, nv);
    } else if (isLouvain) {
        vertices = relabelVerticesFromLouvain(fp, nv);
    } else {
        /** Error out */
        cerr << "Error: could not recognize provided community label file" << endl;
        exit(1);
    }
    fclose (fp);

    // create relabeling map
    unordered_map<vertexId_t, vertexId_t> relabel_map; /** NOTE: could replace this map with array */
    for (length_t i=0; i<nv; i++) {
        relabel_map[vertices[i]] = i;
    }

    // write out relabeled graph to file
    ofstream fout;
    fout.open(relabeledGraphPath);
    string outFileName(relabeledGraphPath);
    bool isSnap = outFileName.find(".txt")==std::string::npos?false:true;
    bool isMarket = outFileName.find(".mtx")==std::string::npos?false:true;
    if (isSnap) {
        printf("Outputting new SNAP graph\n");
        fout << "# Nodes: " << vertices.size() << " " << "Edges: " << ne << "\n";
    } else if (isMarket) {
        printf("Outputting new matrix market graph\n");
        fout << vertices.size() << " " <<  vertices.size() << " " << ne << "\n";
    } else {
        cout << "Unrecognized output graph file type: defaulting to SNAP .txt" << endl;
        printf("Outputting new SNAP graph\n");
        fout << "# Nodes: " << vertices.size() << " " << "Edges: " << ne << "\n";
    }
    vertexId_t relabeledSrc, relabeledDest;
    for (vertexId_t i=0; i<counter; i++) {
        relabeledSrc = relabel_map[src[i]];
        relabeledDest = relabel_map[dest[i]];
        if (isSnap) {
            fout << relabeledSrc << " " << relabeledDest << "\n";
        } else {
            fout << relabeledSrc+1 << " " << relabeledDest+1 << "\n";
        }
    }
    fout.close();

    // off-by-1 if output is mtx
    if (mappingPath != NULL) {
        fout.open(mappingPath);
        fout << "New ID Old ID\n";
        for (vertexId_t i=0; i<vertices.size(); i++)
            fout << i << " " << vertices[i] << "\n";
    }

    printf("Processed %lu vertices, %d edges as input\n", vertices.size(), counter);
    free(src);
    free(dest);
}

// assumes SNAP input
void writeDegreesFile(char* inputGraphPath, char* outputPath) {
    vertexId_t nv;
    length_t ne;
    nv = ne = -1;
    const int MAX_CHARS = 1000;
    char temp[MAX_CHARS];
    char *written;
    FILE *fp = fopen(inputGraphPath, "r");

    // scan for SNAP header comment
    while (nv == -1 || ne == -1) {
        fgets(temp, MAX_CHARS, fp);
        sscanf(temp, "# Nodes: %d Edges: %d\n", &nv,&ne);
    }
    written = fgets(temp, MAX_CHARS, fp);
    while (written != NULL && *temp == '#') { // skip any other comments
        written = fgets(temp, MAX_CHARS, fp);
    }

    length_t counter = 0;
    vector<int> degrees(nv, 0);
    vertexId_t srctemp,desttemp;

    // read in edges
    while(counter<ne)
    {
        sscanf(temp, "%d %d\n", (vertexId_t*)&srctemp, (vertexId_t*)&desttemp);
        degrees[srctemp]++;
        degrees[desttemp]++;
        counter++;
        fgets(temp, MAX_CHARS, fp);
    }
    fclose (fp);

    // count degrees
    vector<pair<vertexId_t, int>> vid_degree_pairs(nv);
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

int main(const int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Too few arguments\n");
        exit(-1);
    }
    char *input_graph_path = argv[1];
    char *output_graph_path = argv[2];
    char *partition_info_path = getOption("-p", argc, argv);
    char *mapping_path = getOption("-m", argc, argv);
    bool get_degrees = hasOption("--degrees", argc, argv);

    clock_t diff;
    clock_t start = clock();
    if (partition_info_path != NULL) {
        relabelGraphSNAP(input_graph_path, output_graph_path, partition_info_path, mapping_path);
    } else if (get_degrees) {
        writeDegreesFile(input_graph_path, output_graph_path);
    } else {
        readGraphSNAP(input_graph_path, output_graph_path, mapping_path);
    }
    diff = clock() - start;
    int msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("Time elapsed: %d seconds %d milliseconds\n", msec/1000, msec%1000);
    return 0;
}




