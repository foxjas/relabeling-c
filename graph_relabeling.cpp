#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

typedef int32_t vertexId_t;
typedef vertexId_t length_t;

using namespace std;

bool hasOption(const char* option, int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
      if (strcmp(argv[i], option) == 0)
          return true;
  }
  return false;
}

char* getOption(const char* option, int argc, char **argv) {
  for (int i = 1; i < argc-1; i++) {
      if (strcmp(argv[i], option) == 0)
          return argv[i+1];
  }
  return NULL;
}

void readGraphSNAP(char* inputGraphPath, char* relabeledGraphPath, char* out_type, char* mappingPath) {
    vertexId_t nv,*src,*dest;
    length_t   ne;
    nv = ne = -1;

    const int MAX_CHARS = 1000;
    char temp[MAX_CHARS];
    FILE *fp = fopen(inputGraphPath, "r");

    // scan for SNAP header comment
    while (nv == -1 || ne == -1) {
    	fgets(temp, MAX_CHARS, fp);
    	sscanf(temp, "# Nodes: %d%*s Edges: %d\n", &nv,&ne);
    }
    while (fgets(temp, MAX_CHARS, fp) && *temp == '#'); // skip any other comments

    src = (vertexId_t *) malloc ((ne ) * sizeof (vertexId_t));
    dest = (vertexId_t *) malloc ((ne ) * sizeof (vertexId_t));
    unordered_set<vertexId_t> vertex_set;

    vertexId_t counter=0;
    vertexId_t srctemp,desttemp;
    // read in edges
    while(counter<ne)
    {
        fgets(temp, MAX_CHARS, fp);
        sscanf(temp, "%d %d\n", (vertexId_t*)&srctemp, (vertexId_t*)&desttemp);
        src[counter]=srctemp;
        dest[counter]=desttemp;
        vertex_set.insert(srctemp);
        vertex_set.insert(desttemp);
        counter++;
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
    bool snap_output = (out_type == NULL || strcmp(out_type, "snap") == 0);
    if (snap_output) {
        printf("Outputting new SNAP graph\n");
        fout << "# Nodes: " << vertices.size() << " " << "Edges: " << ne << "\n";
    } else {
        printf("Outputting new matrix market graph\n");
        fout << vertices.size() << " " <<  vertices.size() << " " << ne << "\n";
    }
    vertexId_t relabeledSrc, relabeledDest;
    for (vertexId_t i=0; i<counter; i++) {
        relabeledSrc = relabel_map[src[i]];
        relabeledDest = relabel_map[dest[i]];
        if (snap_output) {
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

void relabelGraphSNAP(char* inputGraphPath, char* relabeledGraphPath, char* out_type, char* partitionInfoPath, char* mappingPath) {
    vertexId_t nv,*src,*dest;
    length_t   ne;
    nv = ne = -1;

    const int MAX_CHARS = 1000;
    char temp[MAX_CHARS];
    FILE *fp = fopen(inputGraphPath, "r");

    // scan for SNAP header comment
    while (nv == -1 || ne == -1) {
        fgets(temp, MAX_CHARS, fp);
        sscanf(temp, "# Nodes: %d%*s Edges: %d\n", &nv,&ne);
    }
    while (fgets(temp, MAX_CHARS, fp) && *temp == '#'); // skip any other comments

    src = (vertexId_t *) malloc ((ne ) * sizeof (vertexId_t));
    dest = (vertexId_t *) malloc ((ne ) * sizeof (vertexId_t));

    vertexId_t counter=0;
    vertexId_t srctemp,desttemp;
    // read in edges
    while(counter<ne)
    {
        fgets(temp, MAX_CHARS, fp);
        sscanf(temp, "%d %d\n", (vertexId_t*)&srctemp, (vertexId_t*)&desttemp);
        src[counter]=srctemp;
        dest[counter]=desttemp;
        counter++;
    }
    fclose (fp);

    // read in community-partitioned vertices
    vertexId_t vid, i=0;
    vector<vertexId_t> vertices(nv);
    fp = fopen(partitionInfoPath, "r");
    char *written = fgets(temp, MAX_CHARS, fp);
    while (written != NULL && *temp == '#') { // skip comments
        written = fgets(temp, MAX_CHARS, fp);
    }
    while (written != NULL) {
        sscanf(temp, "%d %*s\n", (vertexId_t*)&vid);
        vertices[i] = vid;
        written = fgets(temp, MAX_CHARS, fp);
        i += 1;
    }
    fclose (fp);

    // create relabeling map
    unordered_map<vertexId_t, vertexId_t> relabel_map;
    for (length_t i=0; i<nv; i++) {
        relabel_map[vertices[i]] = i;
    }

    // write out relabeled graph to file
    ofstream fout;
    fout.open(relabeledGraphPath);

    bool snap_output = (out_type == NULL || strcmp(out_type, "snap") == 0);
    if (snap_output) {
        printf("Outputting new SNAP graph\n");
        fout << "# Nodes: " << vertices.size() << " " << "Edges: " << ne << "\n";
    } else {
        printf("Outputting new matrix market graph\n");
        fout << vertices.size() << " " <<  vertices.size() << " " << ne << "\n";
    }
    vertexId_t relabeledSrc, relabeledDest;
    for (vertexId_t i=0; i<counter; i++) {
        relabeledSrc = relabel_map[src[i]];
        relabeledDest = relabel_map[dest[i]];
        if (snap_output) {
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
    char *out_type = getOption("-o", argc, argv);

    clock_t diff;
    clock_t start = clock();
    if (partition_info_path != NULL) {
        relabelGraphSNAP(input_graph_path, output_graph_path, out_type, partition_info_path, mapping_path);
    } else {
        readGraphSNAP(input_graph_path, output_graph_path, out_type, mapping_path);
    }
    diff = clock() - start;
    int msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("Time elapsed: %d seconds %d milliseconds\n", msec/1000, msec%1000);
    return 0;
}




