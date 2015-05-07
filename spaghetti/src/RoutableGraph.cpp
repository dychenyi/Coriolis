
#include "spaghetti/Graph.h"

#include <cmath>

namespace spaghetti{

void RoutableGraph::unroute  ( EdgePredicate edgePredicate ){
    std::vector<EdgeIndex> unrouted;
    for(EdgeIndex e=0; e<edges.size(); ++e)
        if(edgePredicate(edges[e]))
            unrouted.push_back(e);
    for(Net & n : nets){
        unrouteSelectedEdges(n, unrouted);
        unrouteUnusedEdges(n);
    }
}

void RoutableGraph::biroute  ( EdgeCostFunction edgeCostFunction ){
    for(Net & n : nets)
        birouteNet(edgeCostFunction, n);
}

void RoutableGraph::triroute ( EdgeCostFunction edgeCostFunction ){
    for(Net & n : nets)
        trirouteNet(edgeCostFunction, n);
}

void RoutableGraph::rebiroute    ( EdgePredicate edgePredicate, EdgeCostFunction edgeCostFunction ){
    for(Net & n : nets){
        unrouteOverflowedEdges(n, edgePredicate);
        unrouteUnusedEdges(n);
        birouteNet(edgeCostFunction, n);
    }
}

void RoutableGraph::retriroute   ( EdgePredicate edgePredicate, EdgeCostFunction edgeCostFunction ){
    for(Net & n : nets){
        unrouteOverflowedEdges(n, edgePredicate);
        unrouteUnusedEdges(n);
        trirouteNet(edgeCostFunction, n);
    }
}

bool RoutableGraph::isRouted   () const{
    for(Net const & n : nets)
        if(not isNetRouted(n))
            return false;
    return true;
}

bool RoutableGraph::isCorrectlyRouted( EdgePredicate edgePredicate ) const{
    return isRouted() and not overflow(edgePredicate);
}

} // End namespace spaghetti

