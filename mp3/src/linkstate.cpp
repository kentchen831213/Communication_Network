/*
** creat by kent chen
*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<iostream>
#include<fstream>
#include<queue>
#include <bits/stdc++.h>
#include<unordered_map>

using namespace std;

struct node_graph{
    int id;
    map <int, int> neighbor_cost;  // key is neighbor id, value is cost 
};
struct node_dijkstra{
    int next_hop;
    int min_cost;
};

fstream topofile, messagefile, changesfile;
ofstream fpOut;
int file_length;
unordered_map<int, node_graph> graph; // the key is node id, the value is a node_graph struct's, containing neighbor's id and cost
map<pair<int,int>, node_dijkstra> routing_table; // the key is node id and destination id, the value is routing table, nexthop and shortest path cost
map<int,priority_queue<vector<int>>> mins_cost; //key is node id, value is a priority_queue struct, containing the cost, source's id, and corresponding node's id
vector<string> message_output; // store each output message: ex: from source to destination cost 6 hops 2 5 4 message content...
map<pair<int, int>, vector<int>> path; //create a map to record the original path

void build_initial_map(){
    int source, neighbor, cost;
    while(!topofile.eof()){

        topofile>>source>>neighbor>>cost;

        //store information in map
        if(graph.find(source) == graph.end()){
            node_graph source_temp;
            source_temp.id = source;
            source_temp.neighbor_cost[neighbor] = cost;
            graph[source] = source_temp;
        }
        else{
            graph[source].neighbor_cost[neighbor] = cost;
        }
        if(graph.find(neighbor) == graph.end()){
            node_graph source_temp;
            source_temp.id = neighbor;
            source_temp.neighbor_cost[source] = cost;
            graph[neighbor] = source_temp;
        }  
        else{
            graph[neighbor].neighbor_cost[source] = cost;
        }    
    }
}

void Dijkstra(){

    //build up all the node's initial routing table
    for(auto i:graph){
        for(auto j: i.second.neighbor_cost){
            node_dijkstra temp;
            temp.next_hop = j.first;
            temp.min_cost = j.second;
            pair <int,int> temp_key(i.first, j.first);
            routing_table[temp_key] = temp;
            path[temp_key] = {i.first, j.first};
            vector<int> cur_path {-j.second,-j.first, -j.first};
            mins_cost[i.first].push(cur_path);
           
        }
        // add it self to the routing table
        node_dijkstra temp;
        temp.next_hop = i.first;
        temp.min_cost = 0;
        pair <int,int> temp_key(i.first, i.first);
        routing_table[temp_key] = temp;
    }

    // start dijkstra algorithm, build each node's routing table
    for(auto i:graph){
        set<int> visited;
        visited.insert(i.first);

        // record current path
        vector<int> current_path, original_path, pre_path;
        current_path.push_back(i.first);

        while(!mins_cost[i.first].empty()){
           
            // choose current smallest cost path 
            vector<int> top = mins_cost[i.first].top();
            mins_cost[i.first].pop();
            int cur_cost = -top[0]; 
            int source_node = -top[1];
            int cur_node = -top[2];

            // if cur node visited, iterative to next node, else, calculate the cost and update the routing table 
            if(visited.find(cur_node) != visited.end()){
                continue;
            }
            visited.insert(cur_node);
            //add current node to current path
            current_path.push_back(cur_node);
            
            for(auto node:graph[cur_node].neighbor_cost){
                    
                //if current node visited: contiue
                if(visited.find(node.first) != visited.end()){
                    continue;
                }
                // add current node to current path
                current_path.push_back(node.first);

                //if find a new node, add it nexthop and cost to routing table
                if(routing_table.find(make_pair(i.first,node.first)) == routing_table.end()){
                    node_dijkstra temp;
                    temp.next_hop = source_node;
                    temp.min_cost = node.second+cur_cost;
                    pair <int,int> temp_key(i.first, node.first);
                    routing_table[temp_key] = temp;
                    vector<int> cur_path {-temp.min_cost,-source_node, -node.first};
                    mins_cost[i.first].push(cur_path);

                    pair<int, int> pre = {i.first,cur_node};
                    pre_path = path[pre];
                    vector<int> help;
                    for(auto k: pre_path){
                        if(k != cur_node)
                            help.push_back(k);
                        else
                            break;
                    }
                    help.push_back(cur_node);
                    help.push_back(node.first);
                    path[temp_key] = help;
                }
                //if the node already exist in routing table, compare the cost, if current cost smaller, update routing table
                else{
                    pair <int,int> temp_key(i.first, node.first);
                    node_dijkstra temp = routing_table[temp_key];
                    if(temp.min_cost>cur_cost+node.second){
                        node_dijkstra temp;
                        temp.next_hop = source_node;
                        temp.min_cost = node.second+cur_cost;
                        pair <int,int> temp_key(i.first, node.first);
                        routing_table[temp_key] = temp;
                        vector<int> cur_path {-temp.min_cost,-source_node, -node.first};
                        mins_cost[i.first].push(cur_path);
                        
                        pair<int, int> pre = {i.first,cur_node};
                        pre_path = path[pre];
                        vector<int> help;
                        for(auto k: pre_path){
                            if(k != cur_node)
                                help.push_back(k);
                            else
                                break;
                        }
                        help.push_back(cur_node);
                        help.push_back(node.first);
                        path[temp_key] = help;
                    }
                     // Deal with tie breaking
                    else if(temp.min_cost==cur_cost+node.second){
                        pair<int, int> pre = {i.first,cur_node};
                        pre_path = path[pre];
                        vector<int> help;
                        for(auto k: pre_path){
                            if(k != cur_node)
                                help.push_back(k);
                            else
                                break;
                        }
                        help.push_back(cur_node);
                        help.push_back(node.first);
                        original_path = path[temp_key];
                        int org_len = original_path.size()-1;
                        int help_len = help.size()-1;
                        while(org_len>=0 && help_len>=0){
                            if(original_path[org_len]<help[help_len]){
                                break;
                            }
                            else if(help[help_len]<original_path[org_len]){
                                node_dijkstra temp;
                                temp.next_hop = source_node;
                                temp.min_cost = node.second+cur_cost;
                                pair <int,int> temp_key(i.first, node.first);
                                routing_table[temp_key] = temp;
                                vector<int> cur_path {-temp.min_cost,-source_node, -node.first};
                                mins_cost[i.first].push(cur_path);
                                path[temp_key] = help;
                                break;
                            }
                            org_len--;
                            help_len--;
                        }
                    }
                }
                current_path.pop_back();
            }
            current_path.pop_back();
        }    
        current_path.pop_back();
        current_path.clear();
    }
}

void print_table(){

    for(auto i: routing_table){
        fpOut << i.first.second<< " " << i.second.next_hop<< " " << i.second.min_cost<< "\n";
    }
}

void read_message(){
    int source, destination;
    string content, line;
    while(messagefile >> source >> destination){
        getline(messagefile, content);
        auto start = content.find_first_not_of(" ");
        content = content.substr(start);

        //if the destination is not reachable, not go through the routing table
        if(routing_table.find(make_pair(source,destination)) == routing_table.end()){
            fpOut << "from " << source << " to " << destination << " cost infinite hops unreachable message " << content<<"\n";
        }
        else{// go through routing table and find out out all the path hops 
            pair <int,int> temp_key(source, destination);
            node_dijkstra temp = routing_table[temp_key];
            int cost = temp.min_cost;
            int next = temp.next_hop;
            string path = to_string(source);
            while(next != destination){
                pair <int,int> temp_key(next , destination);
                node_dijkstra temp = routing_table[temp_key];
                path += " "+to_string(next);
                next = temp.next_hop;
            }
            fpOut << "from " << source << " to " << destination << " cost " << cost << " hops " << path << " message " << content<<"\n";
        }
    }
}

void change_table(){
    int source, neighbor, cost;
    string line;
    string delimiter = " ";
    while(changesfile >> source >> neighbor >> cost){

        // If the cost is -999, delete the corressponding path
        if(cost == -999){
            graph[source].neighbor_cost.erase(neighbor);
            graph[neighbor].neighbor_cost.erase(source);
        }
        else{

            if(graph.find(source) == graph.end()){
                node_graph source_temp;
                source_temp.id = source;
                source_temp.neighbor_cost[neighbor] = cost;
                graph[source] = source_temp;
            }
            else{
                graph[source].neighbor_cost[neighbor] = cost;
            }

            if(graph.find(neighbor) == graph.end()){
                node_graph source_temp;
                source_temp.id = neighbor;
                source_temp.neighbor_cost[source] = cost;
                graph[neighbor] = source_temp;
            }  
            else{
                graph[neighbor].neighbor_cost[source] = cost;
            } 
            
        }
        messagefile.clear();
        messagefile.seekg(0,messagefile.beg);  
        routing_table.clear();
        Dijkstra();
        print_table();
        read_message();
    }
}

int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }
    string file = argv[1];
    string message_file = argv[2];
    string change_file = argv[3];
    fpOut.open("output.txt");
    topofile.open(file);
    messagefile.open(message_file);
    changesfile.open(change_file);

    build_initial_map();
    Dijkstra();
    print_table();
    read_message();
    change_table();

    topofile.close();
    messagefile.close();
    changesfile.close();
    fpOut.close();
    return 0;
}