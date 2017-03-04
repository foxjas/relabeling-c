#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

#include "utils.h"

using namespace std;

void readGraph(char* inputGraphPath, char* relabeledGraphPath, char* mappingPath, int undirected) {
    vertexId_t nv = -1;
    length_t ne = -1;
    const int MAX_CHARS = 1000;
    char temp[MAX_CHARS];
    char *written;
    FILE *fp = fopen(inputGraphPath, "r");

    string inFileName(inputGraphPath);
    bool isSnapInput = inFileName.find(".txt")==std::string::npos?false:true;
    bool isMarketInput = inFileName.find(".mtx")==std::string::npos?false:true;

    // process input file header
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

    vector<pair<vertexId_t, vertexId_t> >  edges(ne);
    unordered_set<vertexId_t> vertex_set;
    vertexId_t counter=0;
    vertexId_t srctemp,desttemp;

    // read in edges
    while(counter<ne)
    {
        sscanf(temp, "%d %d\n", (vertexId_t*)&srctemp, (vertexId_t*)&desttemp);
        if (isMarketInput) {
            srctemp-=1;
            desttemp-=1;
        }
        if (undirected) {
            edges[counter]= make_pair(min(srctemp, desttemp), max(srctemp, desttemp));
        } else {
            edges[counter] = make_pair(srctemp, desttemp);
        }
        vertex_set.insert(srctemp);
        vertex_set.insert(desttemp);
        counter++;
        fgets(temp, MAX_CHARS, fp);
    }
    fclose (fp);
    printf("Original input: %d vertices, %d edges\n", vertex_set.size(), edges.size());
    assert(edges.size() == ne);

    // convert to undirected edges, and remove potential duplicates
    vector<pair<vertexId_t, vertexId_t> > edges_final;
    if (undirected) {
        sort(edges.begin(), edges.end(), sortByPairAsec);
        vertexId_t prev_first = -1;
        vertexId_t prev_second = -1;
        vertexId_t first, second;
        int duplicates = 0;
        for (vector<pair<vertexId_t, vertexId_t> >::iterator pair = edges.begin(); pair != edges.end(); pair++) {
            first = pair->first;
            second = pair->second;
            if (first == prev_first && second == prev_second) {
                duplicates += 1;
            } else {
                edges_final.push_back(*pair);
                if (undirected == 2) {
                    edges_final.push_back(make_pair(second, first));
                }
            }
            prev_first = first;
            prev_second = second;
        }
        ne = edges_final.size();
        printf("Removed %d duplicate edges in conversion to undirected\n", duplicates);
    } else {
        edges_final = edges;
    }

    vector<pair<vertexId_t, vertexId_t> >().swap(edges); // deallocates edges

    // sort graph vertices and use to create relabeling map
    nv = vertex_set.size();
    vector<vertexId_t> vertices(vertex_set.begin(), vertex_set.end());
    sort(vertices.begin(), vertices.end());
    unordered_map<vertexId_t, vertexId_t> relabel_map;
    for (length_t i=0; i<nv; i++) {
    	relabel_map[vertices[i]] = i;
    }

    // relabel & write out relabeled graph to file
    ofstream fout;
    fout.open(relabeledGraphPath);
    string outFileName(relabeledGraphPath);
    bool isSnapOutput = outFileName.find(".txt")==std::string::npos?false:true;
    bool isMarketOutput = outFileName.find(".mtx")==std::string::npos?false:true;
    if (isSnapOutput) {
        printf("Outputting new SNAP graph\n");
        fout << "# Nodes: " << vertices.size() << " " << "Edges: " << ne << "\n";
    } else if (isMarketOutput) {
        printf("Outputting new matrix market graph\n");
        fout << vertices.size() << " " <<  vertices.size() << " " << ne << "\n";
    } else {
        cout << "Unrecognized output graph file type: defaulting to SNAP .txt" << endl;
        printf("Outputting new SNAP graph\n");
        fout << "# Nodes: " << vertices.size() << " " << "Edges: " << ne << "\n";
    }
    vertexId_t relabeledSrc, relabeledDest;
    for (vertexId_t i=0; i<ne; i++) {
        relabeledSrc = relabel_map[edges_final[i].first];
        relabeledDest = relabel_map[edges_final[i].second];
        // assert(relabeledSrc >= 0 && relabeledSrc < nv);
        // assert(relabeledDest >= 0 && relabeledDest < nv);
        if (isSnapOutput) {
            fout << relabeledSrc << " " << relabeledDest << "\n";
        } else {
            fout << relabeledSrc+1 << " " << relabeledDest+1 << "\n";
        }
    }
    fout.close();

    // write out re-mapping file
    if (mappingPath != NULL) {
        fout.open(mappingPath);
        fout << "New ID Old ID\n";
        for (vertexId_t i=0; i<vertices.size(); i++)
            fout << i << " " << vertices[i] << "\n";
    }

    printf("Processed %lu vertices, %d edges\n", vertices.size(), ne);
}

vector<vertexId_t> relabelVerticesFromInfomap(char *fpath, int nv) {
    // read in community-partitioned vertices
    FILE *com_fp = fopen(fpath, "r");
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
    fclose(com_fp);
    return vertices;
}

vector<vertexId_t> relabelVerticesFromSCD(char *fpath, int nv) {

    ifstream inFile;
    inFile.open(fpath);
    vertexId_t vid, i=0;
    vector<vertexId_t> vertices(nv);

    while (inFile >> vid) {
        vertices[i] = vid;
        i += 1;
    }
    printf("i: %d\n", i);
    return vertices;
}

vector<vertexId_t> relabelVerticesFromLouvain(char *fpath, int nv) {
    vector<pair<vertexId_t, int> > vertex_labels;
    vertex_labels.reserve(nv);
    vertexId_t a_prev = -1;
    vertexId_t a;
    int b;
    char *written;
    const int MAX_CHARS = 100;
    char temp[MAX_CHARS];
    FILE *com_fp = fopen(fpath, "r");
    
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
    for (vector<pair<vertexId_t, int> >::iterator pair = vertex_labels.begin(); pair != vertex_labels.end(); pair++) {
        final.push_back((*pair).first);
    }
    fclose(com_fp);
    assert(final.size() == nv);
    return final;
}

void relabelGraph(char* inputGraphPath, char* relabeledGraphPath, char* partitionInfoPath, char* mappingPath) {
    vertexId_t nv,*src,*dest;
    length_t   ne;
    nv = ne = -1;
    const int MAX_CHARS = 1000;
    char temp[MAX_CHARS];
    char *written;
    FILE *fp = fopen(inputGraphPath, "r");

    string inFileName(inputGraphPath);
    bool isSnapInput = inFileName.find(".txt")==std::string::npos?false:true;
    bool isMarketInput = inFileName.find(".mtx")==std::string::npos?false:true;

    // process input file header
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

    src = (vertexId_t *) malloc ((ne ) * sizeof (vertexId_t));
    dest = (vertexId_t *) malloc ((ne ) * sizeof (vertexId_t));
    vertexId_t counter=0;
    vertexId_t srctemp,desttemp;

    // read in edges
    while(counter<ne)
    {
        sscanf(temp, "%d %d\n", (vertexId_t*)&srctemp, (vertexId_t*)&desttemp);
        if (isMarketInput) {
            srctemp -= 1;
            desttemp -= 1;
        }
        src[counter]=srctemp;
        dest[counter]=desttemp;
        counter++;
        fgets(temp, MAX_CHARS, fp);
    }
    fclose (fp);

    vector<vertexId_t> vertices;
    string comFileName(partitionInfoPath);
    bool isInfomap = comFileName.find(".clu")==std::string::npos?false:true;
    bool isLouvain = comFileName.find(".tree")==std::string::npos?false:true;
    bool isSCD = comFileName.find(".dat")==std::string::npos?false:true;
    // read in community-partitioned vertices
    if (isInfomap) {
        vertices = relabelVerticesFromInfomap(partitionInfoPath, nv);
    } else if (isLouvain) {
        vertices = relabelVerticesFromLouvain(partitionInfoPath, nv);
    } else if (isSCD) {
        vertices = relabelVerticesFromSCD(partitionInfoPath, nv);
    } else {
        /** Error out */
        cerr << "Error: could not recognize provided community label file" << endl;
        exit(1);
    }

    // create relabeling map
    unordered_map<vertexId_t, vertexId_t> relabel_map; /** NOTE: could replace this map with array */
    for (length_t i=0; i<nv; i++) {
        relabel_map[vertices[i]] = i;
    }

    // write out relabeled graph to file
    ofstream fout;
    fout.open(relabeledGraphPath);
    string outFileName(relabeledGraphPath);
    bool isSnapOutput = outFileName.find(".txt")==std::string::npos?false:true;
    bool isMarketOutput = outFileName.find(".mtx")==std::string::npos?false:true;
    if (isSnapOutput) {
        printf("Outputting new SNAP graph\n");
        fout << "# Nodes: " << vertices.size() << " " << "Edges: " << ne << "\n";
    } else if (isMarketOutput) {
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
        assert(relabeledSrc >= 0 && relabeledSrc < nv);
        assert(relabeledDest >= 0 && relabeledDest < nv);
        if (isSnapOutput) {
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
    bool get_degrees = hasOption("--degrees", argc, argv);
    bool get_com_stats = hasOption("--community", argc, argv);
    bool edge_dups = hasOption("--duplicates", argc, argv);
    int undirected = getIntOption("--undirected", argc, argv);

    clock_t diff;
    clock_t start = clock();
    if (partition_info_path != NULL) {
        relabelGraph(input_graph_path, output_graph_path, partition_info_path, mapping_path);
    } else if (get_degrees) {
        writeDegreesFile(input_graph_path, output_graph_path);
    } else if (get_com_stats) {
        writeCommunityStats(input_graph_path, output_graph_path);
    } else if (edge_dups) {
        printf("Duplicated eges: %d\n", checkDuplicateEdges(input_graph_path));
    } else {
        readGraph(input_graph_path, output_graph_path, mapping_path, undirected);
    }
    diff = clock() - start;
    int msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("Time elapsed: %d seconds %d milliseconds\n", msec/1000, msec%1000);
    return 0;
}




