
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
        unrouteOverflowEdges(n, edgePredicate);
        unrouteUnusedEdges(n);
        birouteNet(edgeCostFunction, n);
    }
}

void RoutableGraph::retriroute   ( EdgePredicate edgePredicate, EdgeCostFunction edgeCostFunction ){
    for(Net & n : nets){
        unrouteOverflowEdges(n, edgePredicate);
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

bool RoutableGraph::overflow ( EdgePredicate edgePredicate ) const{
    for(EdgeProperties const & e : edges)
        if(edgePredicate(e))
            return true;
    return false;
}

bool RoutableGraph::isCorrectlyRouted( EdgePredicate edgePredicate ) const{
    return isRouted() and not overflow(edgePredicate);
}

Cost RoutableGraph::avgCost   ( EdgeEvalFunction edgeEvalFunction ) const{
    Cost tot = 0.0f;
    for(Edge const & e : edges)
        tot += edgeEvalFunction(e);
    if(edges.size() == 0) return 0.0f;
    else                  return tot/edges.size();
}
Cost RoutableGraph::maxCost   ( EdgeEvalFunction edgeEvalFunction ) const{
    Cost tot = 0.0f;
    for(Edge const & e : edges)
        tot = std::max(edgeEvalFunction(e), tot);
    return tot;
}
Cost RoutableGraph::qAvgCost  ( EdgeEvalFunction edgeEvalFunction ) const{
    Cost tot = 0.0f;
    for(Edge const & e : edges){
        Cost cur = edgeEvalFunction(e);
        tot += cur*cur;
    }
    if(edges.size() == 0) return 0.0f;
    else                  return std::sqrt(tot/edges.size());
}


} // End namespace spaghetti

