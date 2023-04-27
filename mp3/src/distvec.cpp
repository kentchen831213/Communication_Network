/*
** creat by kent chen
*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cassert>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <algorithm>
using namespace std;


struct node_graph {
    int neigh;
    int weight;
    node_graph(int neigh, int weight) : neigh(neigh), weight(weight) {}
};
// <des, weight>
unordered_map<int, vector<node_graph>> graph;
// store distance from different start points
unordered_map<int, unordered_map<int, tuple<int, int>>>  distance_matrix;
const int INF = 9999;

unordered_map<int, tuple<int, int>> distance_vector_algorithm(int source, int num_nodes, ofstream& output) {
    unordered_map<int, tuple<int, int>> distance;
    vector<int> node_list;
    for (const auto& node : graph) {
        distance[node.first] = make_tuple(-1, INF);
    }

    distance[source] = make_tuple(source, 0);

    // calculate distance
    for (int i = 0; i < num_nodes - 1; ++i) {
        for (auto item : graph) {
            int u = item.first;
            for (auto node : item.second) {
                int v = node.neigh;
                int weight = node.weight;

                if (get<1>(distance[u]) != INF && get<1>(distance[u]) + weight < get<1>(distance[v])) {
                    distance[v] = make_tuple(u, get<1>(distance[u]) + weight);
                }
                if (get<1>(distance[u]) != INF && get<1>(distance[u]) + weight == get<1>(distance[v]) && u < get<0>(distance[v])){
                    distance[v] = make_tuple(u, get<1>(distance[u]) + weight);
                }
            }
        }
    }


    for (auto node: graph){
        node_list.push_back(node.first);
    }
    sort(node_list.begin(), node_list.end());
    // Print forwarding table for the source node
    // output << "topology entries for node " << source << ":\n";
    for (const auto& node: node_list) {
        pair<int, tuple<int, int>>item = make_pair(node, distance[node]);
        int dest = item.first;
        int next_hop = get<0>(item.second);
        // calculate optimal path
        while (get<0>(distance[next_hop]) != source && next_hop != -1){
            next_hop = get<0>(distance[next_hop]);
        }
        int path_cost = get<1>(item.second);
        if (next_hop == source){
            next_hop = dest;
        }
        if (path_cost != INF) {
            output << dest << " " << next_hop << " " << path_cost << "\n";
        }
    }
    return distance;
}

void topology_output(ofstream& output){
    vector<int> sorted_node;
    int nums_nodes = graph.size();
    for (auto node: graph){
        sorted_node.push_back(node.first);
    }
    sort(sorted_node.begin(), sorted_node.end());
    // Call the distance_vector_algorithm for each node in the graph
    for (const auto& src_node : sorted_node) {
        distance_matrix[src_node] = distance_vector_algorithm(src_node, nums_nodes, output);
    }
}

void message_output(ofstream& output, string filename){
    ifstream message_file(filename);
    int src, dest;
    vector<tuple<int, int, string>> messages;
    string message_text;
    while (message_file >> src >> dest) {
        getline(message_file, message_text);
        messages.emplace_back(src, dest, message_text.substr(1));
    }
    message_file.close();
    for (auto message: messages){
        vector<int> tmp_result;
        src = get<0>(message);
        dest = get<1>(message);
        unordered_map<int, tuple<int, int>> distance = distance_matrix[src];
        int cost = get<1>(distance[dest]);
        // unreachable check
        if (cost == INF){
            output <<"from "<<src<<" to "<<dest<< " cost infinite hops unreachable ";
        }
        else{ // when the node is reachable
            int next_hop = dest;
            while (get<0>(distance[next_hop]) != src){
                next_hop = get<0>(distance[next_hop]);
                tmp_result.push_back(next_hop);
            }
            tmp_result.push_back(src);
            reverse(tmp_result.begin(), tmp_result.end());
            output <<"from "<<src<<" to "<<dest<< " cost " << cost << " hops ";
            for (auto path:tmp_result){
                output<<path<<" ";
            }
        }
        output<<"message "<<get<2>(message)<<"\n";
    }

}

void change_graph(int source, int destination, int weight){
    int del_idx = -1;
    int found = 0;
    for (int i=0; i<graph[source].size(); i++){
        if (graph[source][i].neigh == destination){
            // find this pair
            found = 1;
            if (weight >= 0){
                graph[source][i].weight = weight;
            }
            else {
                del_idx = i;
            }
            break;
        }
    }
    if (found == 0 and weight >=0){
        graph[source].push_back(node_graph(destination, weight));
    }
    if (del_idx != -1){
        graph[source].erase(graph[source].begin()+del_idx);
    }   
}

int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }

    set<int> unique_v;
    vector<int> sorted_node;
    ifstream infile; 
    string file = argv[1];
    infile.open(file.data());
    assert(infile.is_open());  
    string s;

    while(getline(infile,s))
    {   
        vector<string> v;
        istringstream ss(s);
        string del;
    
        while(getline(ss, del, ' ')) {
            v.push_back(del);
        }
        graph[stoi(v[0])].push_back( node_graph(stoi(v[1]),stoi(v[2])) );
        graph[stoi(v[1])].push_back( node_graph(stoi(v[0]),stoi(v[2])) );
    }
    infile.close();

    // Open the output file
    ofstream output("output.txt");
    string message_file = argv[2];
    string change_file = argv[3];
    ifstream change(change_file);
    int src, dest, weight;
    topology_output(output);
    
    message_output(output, message_file);
    while (change >> src >> dest >> weight) {
        change_graph(src, dest, weight);
        change_graph(dest, src, weight);

        distance_matrix.clear();
        topology_output(output);
        message_output(output, message_file);
    }

    // Close the output file
    output.close();

    return 0;
}
