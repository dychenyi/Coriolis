
#include "spaghetti/Graph.h"

#include <unordered_map>
#include <unordered_set>
#include <queue>


namespace spaghetti{

// Unroute edges that are connected to only one endpoint (typically after unrouting overflowed edges)
void Graph::unrouteUnusedEdges ( Net & n ){
    // Build a map with the number of accesses of each vertex (being connected to a pin counts)
    std::unordered_map<VertexIndex, int> connectedCounts;
    // Edges used by the net (could as well use a sorted vector)
    std::unordered_set<EdgeIndex>        routing(n.routing.begin(), n.routing.end());
    // Active set and removed vertex set
    std::vector<VertexIndex> toProcess, removedVertices;

    // Increments the counter for the vertex and adds it to the active set
    auto increment = [&](VertexIndex v){
        toProcess.push_back(v);
        auto it = connectedCounts.find(v);
        if(it != connectedCounts.end()){
            ++(it->second);
        }
        else{
            connectedCounts.emplace(v, 1);
        }
    };

    for(auto const & comp : n.initialComponents){
        for(VertexIndex const v : comp){
            increment(v);
        }
    }
    for(EdgeIndex e : n.routing){
        increment(edges[e].vertices[0]);
        increment(edges[e].vertices[1]);
    }

    // If a vertex has only one access, it is an end of line: we can remove it from the map and delete the edges there
    while(not toProcess.empty()){
        VertexIndex v = toProcess.back();
        toProcess.pop_back();
        auto source_it = connectedCounts.find(v);
        if(source_it != connectedCounts.end() and source_it->second <= 1){
            for(auto neigh : neighbours(v)){
                auto edge_it = routing.find(neigh.edge);
                if( edge_it != routing.end() ){
                    routing.erase(edge_it); // Just for performance
                    auto vertex_it = connectedCounts.find(neigh.vertex);
                    if(vertex_it != connectedCounts.end()){
                        --(vertex_it->second);
                        if(vertex_it->second <= 1) // If it can be removed, we are going to process it later
                            toProcess.push_back(neigh.vertex);
                    }
                }
            }
            connectedCounts.erase(source_it); // Won't process it twice
            removedVertices.push_back(v);
        }
    }

    // Create the new routing for the net and update the demands for the graph's edges
    std::vector<EdgeIndex> nextRouting;
    std::unordered_set<VertexIndex> removedVerticesSet(removedVertices.begin(), removedVertices.end());
    for(EdgeIndex e : n.routing){
        // No incident edge has been removed
        if(  removedVerticesSet.count(edges[e].vertices[0]) == 0
         and removedVerticesSet.count(edges[e].vertices[1]) == 0 ){
            nextRouting.push_back(e);
        }
        else{
            edges[e].capacity -= n.demand;
        }
    }
    n.routing.swap(nextRouting);

}

// More complex technique: unroute parts of the net that are relatively simple.
// That is, every path between two of the initial components, but not parts of the routing that connect 3 or more
// Unrouting them too makes it possible for the router to find new paths connecting several components
void Graph::unrouteSimplePaths ( Net & n ){

}

// Dijsktra-like algorithms for routing
// They are multi-source and multi-sink, flooding the graph from every component until they meet in the middle
// Biroute is the classical multi-source multi-sink algorithm; it routes one path at a time
// Triroute is a new algorithm that take three components into account at a time, effectively building a better Steiner tree

void Graph::biroute  ( EdgeCostFunction, VertexCostFunction, Net & n ){
    // For each connected component, maintain a map of reached vertices
    std::vector<std::unordered_map<VertexIndex, EdgeIndex> > reachedVertices;
    // Maintain a map 
    std::unordered_multimap<VertexIndex, int> reachingComponents;
    
}

void Graph::triroute ( EdgeCostFunction, VertexCostFunction, Net & n ){
}

} // End namespace spaghetti

