
#include "spaghetti/Graph.h"
#include "spaghetti/UnionFindMap.h"
#include "hurricane/Error.h"

#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <cassert>
#include <algorithm>

namespace spaghetti{

using Hurricane::Error;

void Net::selfcheck() const{
    if(demand <= 0)
        throw Error("The net has non-positive demand\n");
    std::vector<EdgeIndex> route = routing;
    std::sort(route.begin(), route.end());
    if(std::unique(route.begin(), route.end()) != route.end())
        throw Error("The net has duplicate edges\n");
}

void Graph::selfcheck() const{
    for(VertexIndex v=0; v<vertices.size(); ++v){
        for(EdgeIndex e : vertices[v].edges){
            assert(e == nullEdgeIndex or e < edges.size());
            if(e != nullEdgeIndex)
                assert(edges[e].vertices[0] == v or edges[e].vertices[1] == v);
        }
    }
    for(EdgeIndex e=0; e<edges.size(); ++e){
        for(VertexIndex v : edges[e].vertices){
            assert(v == nullVertexIndex or v < vertices.size());
            if(v != nullVertexIndex)
                assert(
                    vertices[v].edges[0] == e
                 or vertices[v].edges[1] == e
                 or vertices[v].edges[2] == e
                 or vertices[v].edges[3] == e
                );
        }
    }
}

std::array<Graph::NeighbourAccess, 4> Graph::neighbours(VertexIndex v) const{
    std::array<NeighbourAccess, 4> ret;
    for(int i=0; i<4; ++i){
        ret[i].edge = vertices[v].edges[i];
        if(ret[i].edge != nullEdgeIndex){
            Edge cur_edge = edges[ret[i].edge];
            ret[i].vertex = v ^ cur_edge.vertices[0] ^ cur_edge.vertices[1];
        }
        else
            ret[i].vertex = nullVertexIndex;
    }
    return ret;
}

std::vector<std::vector<VertexIndex> > Graph::getConnectedComponents( Net const & n) const{
    std::vector<VertexIndex> vertices;
    UnionFindMap<VertexIndex> UF;
    for(auto const & comp : n.initialComponents){
        if(not comp.empty()){
            VertexIndex repr = comp.front();
            for(VertexIndex v : comp)
                UF.merge(v, repr);
        }
    }
    for(EdgeIndex e : n.routing)
        UF.merge(edges[e].vertices[0], edges[e].vertices[1]);

    std::vector<std::vector<VertexIndex> > ret = UF.getConnectedComponents();

    // Verify that every component is connected to at least one of the initialComponents (that is, we get at most one component if we merge all initial ones)
    // i.e. verify that all parts of the net are connected to one of the actual pins
    VertexIndex pinsRepr = nullVertexIndex;
    for(auto const & comp : n.initialComponents)
        if(not comp.empty()){
            VertexIndex repr = comp.front();
            if(pinsRepr == nullVertexIndex)
                pinsRepr = repr;
            UF.merge(repr, pinsRepr);
        }
    assert(UF.getConnectedComponents().size() <= 1);

    return ret;
}

void Graph::unrouteOverflowEdges ( Net & n, EdgePredicate edgePredicate ){
    std::vector<EdgeIndex> newRouting;
    for(EdgeIndex e : n.routing){
        if(edgePredicate(edges[e])){
            edges[e].demand -= n.demand;
        }
        else{
            newRouting.push_back(e);
        }
    }
    n.routing.swap(newRouting);
}

void Graph::unrouteSelectedEdges ( Net & n, std::vector<EdgeIndex> const & sortedEdges ){
    std::vector<EdgeIndex> newRouting;
    for(EdgeIndex e : n.routing){
        auto it = std::lower_bound(sortedEdges.begin(), sortedEdges.end(), e);
        if(it == sortedEdges.end() or *it != e)
            newRouting.push_back(e);
    }
    n.routing.swap(newRouting);
}

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
            edges[e].demand -= n.demand;
        }
    }
    n.routing.swap(nextRouting);

}

// More complex technique: unroute parts of the net that are relatively simple.
// That is, every path between two of the initial components, but not parts of the routing that connect 3 or more
// Unrouting them too makes it possible for the router to find new paths connecting several components
void Graph::unrouteSimplePaths ( Net & n ){

}

bool Graph::isNetRouted(Net const & n) const{
    return (getConnectedComponents(n).size() <= 1);
}

// Dijsktra-like algorithms for routing
// They are multi-source and multi-sink, flooding the graph from every component until they meet in the middle
// Biroute is the classical multi-source multi-sink algorithm; it routes one path at a time
// Triroute is a new algorithm that take three components into account at a time, effectively building a better Steiner tree

struct ReachedInfo{
    Cost      cost;
    EdgeIndex edge;

    bool operator<(ReachedInfo const o) const{ // Inverted for priority queues
        return cost > o.cost;
    }

    ReachedInfo(EdgeIndex from, Cost c) : cost(c), edge(from) {}
};

struct QueueInfo : ReachedInfo{
    VertexIndex dest;
    int componentId;

    QueueInfo(VertexIndex to, EdgeIndex from, Cost c, int comp) : ReachedInfo(from, c), dest(to), componentId(comp){
        assert(to != nullVertexIndex);
    }
};

void Graph::birouteNet  ( EdgeCostFunction edgeCostFunction, Net & n ){
    n.selfcheck();
    /*
     * Basic algorithm: flood the graph from every component
     * Events are stored in a priority_queue, reached vertices in a map
     * When an optimal path is found (to 2 or 3 components from the same vertex), backtrack 
     * Then create a new component reaching the newly connected components: no need to clean the priority queue
     */

    typedef std::unordered_map<VertexIndex, ReachedInfo> ReachedMap;
    // For each connected component, maintain a map of reached vertices
    std::vector<std::unordered_map<VertexIndex, ReachedInfo> > reachedVertices;
    // Maintain a map of reached vertices and the corresponding components
    std::unordered_map<VertexIndex, int> reachingComponent;
    int componentsCount=0;

    std::priority_queue<QueueInfo> events;

    // Add a new component
    auto pushComponent = [&](std::vector<VertexIndex> const & vertices){
        // TODO: uniquify (backtracking duplicates some common vertices)
        int componentId = reachedVertices.size();
        reachedVertices.push_back(ReachedMap());
        for(VertexIndex v : vertices){
            assert(v != nullVertexIndex);
            events.push(QueueInfo(v, nullEdgeIndex, 0.0f, componentId));
        }
        ++componentsCount;
    };

    // For a particular component, get the path to this vertex (provided it has been reached)
    auto backtrack = [&](VertexIndex from, int componentId, std::vector<VertexIndex> & component){
        std::vector<EdgeIndex> ret;
        VertexIndex cur = from;
        while(true){
            component.push_back(cur);
            EdgeIndex e = reachedVertices[componentId].at(cur).edge;
            if(e == nullEdgeIndex)
                break;
            n.routing.push_back(e);
            edges[e].demand += n.demand;
            cur = cur ^ edges[e].vertices[0] ^ edges[e].vertices[1]; // Get the one not equal to cur
        }
    };

    std::vector<std::vector<VertexIndex> > connectedComponents = getConnectedComponents(n);
    for(auto const & comp : connectedComponents)
        pushComponent(comp);

    while(not events.empty() and componentsCount > 1){
        QueueInfo cur = events.top();
        events.pop();

        if(reachedVertices[cur.componentId].count(cur.dest) != 0) continue; // Already visited for this component

        // Mark as visited
        reachedVertices[cur.componentId].emplace(cur.dest, static_cast<ReachedInfo>(cur));

        // Verify if we can materialize a route i.e. it was visited by another component
        auto reachedIt = reachingComponent.find(cur.dest);
        if(reachedIt != reachingComponent.end()){ // Found a route: merge the components with this route
            int cId1=cur.componentId, cId2=reachedIt->second;
            std::vector<VertexIndex> newComponent;
            newComponent.insert(newComponent.end(), connectedComponents[cId1].begin(), connectedComponents[cId1].end());
            newComponent.insert(newComponent.end(), connectedComponents[cId2].begin(), connectedComponents[cId2].end());
            backtrack(cur.dest, cId1, newComponent);
            backtrack(cur.dest, cId2, newComponent);
            componentsCount -= 2;
            pushComponent(newComponent);

            // Cleanup (release unused memory)
            reachedVertices[cId1].clear();
            reachedVertices[cId2].clear();
            connectedComponents[cId1].clear();
            connectedComponents[cId2].clear();
        }
        else{
            // Mark as visited in the shared structure
            reachingComponent.emplace(cur.dest, cur.componentId);
            // Push the neighbours with their cost to the queue
            for(auto neigh : neighbours(cur.dest)){
                if(neigh.vertex == nullVertexIndex) continue;
                assert(neigh.edge != nullEdgeIndex);
                events.push(QueueInfo(
                    neigh.vertex, neigh.edge,
                    cur.cost + edgeCostFunction(edges[neigh.edge], n.demand), cur.componentId)
                );
            }
        }
    }

    if(componentsCount > 1)
        throw Error("The maze router has been unable to find a path for the net. Are you sure that the graph is connected?\n");
}

// TODO: trirouting is not the same as birouting
// We want to keep track of all triroutes (and even biroutes) found, then when we are sure that no better route exists chose the best one
// Possible ways: keep track of all positions with a triroute and the length (priority queue?)
void Graph::trirouteNet ( EdgeCostFunction, Net & n ){
}

} // End namespace spaghetti

