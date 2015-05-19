

#include "coloquinte/circuit_helper.hxx"
#include "spaghetti/Grid.h"

#include "hurricane/Error.h"
#include "crlcore/Utilities.h"

namespace spaghetti{

void BidimensionalGrid::steinerRouteNet ( EdgeCostFunction edgeCostFunction, Net & n ){
    using namespace coloquinte;

    bool singleVertexComponents = true;
    for(auto const & comp : n.initialComponents){
        if(comp.size() > 2 or (comp.size() == 2 and getCoord(comp[0]) != getCoord(comp[1]) )){
            singleVertexComponents = false;
        }
    }
    if(not n.routing.empty() or not singleVertexComponents){
        std::cerr << Hurricane::Error("Trying to route with Steiner a net with non-ponctual components or partially routed") << std::endl;
        birouteNet( edgeCostFunction, n );
        return;
    }

    // TODO: consider vertical pins as well since my algorithm can handle it
    auto components = getConnectedComponents(n);
    if(components.size() <= 1) return;

    // Create the Steiner tree's topology considering a single element in each component
    std::vector<point<int_t> > points;
    for(auto const & comp : components){
        GridCoord c = getCoord(comp[0]);
        points.emplace_back(c.x, c.y);
    }
    auto topology = get_RSMT_horizontal_topology(points, 8);

    // Route it i.e. create each edge using a Z-routing of the Steiner tree
    // From the horizontal topology? Enables H-routes to be done, but full H-routes support is difficult
    // Greedily from the horizontal topology -> will support most H-routes and reuse existing Z-routes for them

    // First, extract all mandatory vertical edges i.e. the edge without optional Z-routes at the end
    typedef std::pair<unsigned, unsigned> CCPair;
    std::vector<CCPair> minmaxs(points.size());
    for(index_t i=0; i<points.size(); ++i){
        minmaxs[i] = CCPair(points[i].y_, points[i].y_);
    }
    for(auto const e : topology){
        if(minmaxs[e.second].first > minmaxs[e.first].second){
            minmaxs[e.second].first = minmaxs[e.first].second;
        }
        if(minmaxs[e.second].second < minmaxs[e.first].first){
            minmaxs[e.second].second = minmaxs[e.first].first;
        }
    }

    std::vector<EdgeIndex> toRoute;

    // Second, materialize those edges
    for(index_t i=0; i<points.size(); ++i){
        for(unsigned y=minmaxs[i].first; y<minmaxs[i].second; ++y)
            toRoute.push_back(getVerticalEdgeIndex(points[i].x_, y));
        toRoute.push_back(getTurnEdgeIndex(points[i].x_, points[i].y_));
    }

    // Third, route what remains using horizontal wires or Z-routes
    for(auto const edge : topology){ // Route from component edge.first to component edge.second
        unsigned x1 = points[edge.first ].x_;
        unsigned x2 = points[edge.second].x_;
        auto mm1 = minmaxs[edge.first ];
        auto mm2 = minmaxs[edge.second];
        unsigned xmin=std::min(x1, x2), xmax=std::max(x1, x2);

        // Now route between the vertical lines (x1, mm1) to (x2, mm2)
        assert(mm1.first <= mm2.second and mm2.first <= mm1.second); // Overlap on y: route an horizontal wire
        Cost minCost = std::numeric_limits<Cost>::infinity();
        unsigned bestY = std::numeric_limits<unsigned>::max();
        for(unsigned y=std::max(mm1.first, mm2.first); y<=std::max(mm1.second, mm2.second); ++y){
            Cost totCost = 0.0f;
            for(unsigned x=xmin; x<xmax; ++x)
                totCost += edgeCostFunction(getHorizontalEdge(x, y), n);
            if(totCost < minCost){
                bestY = y;
                minCost = totCost;
            }
        }
        assert(bestY < ydim and std::isfinite(minCost));
        for(unsigned x=xmin; x<xmax; ++x)
            toRoute.push_back(getHorizontalEdgeIndex(x, bestY));
        toRoute.push_back(getTurnEdgeIndex(xmin, bestY));
        toRoute.push_back(getTurnEdgeIndex(xmax, bestY));
    }

    // Uniquify
    std::sort(toRoute.begin(), toRoute.end());
    toRoute.resize(std::distance(toRoute.begin(), std::unique(toRoute.begin(), toRoute.end())));

    for(EdgeIndex e : toRoute){
        routeEdge(e, n);
    }
    // Cleanup the routing: remove edges redundancy due to non-ponctual components
    unrouteUnusedEdges(n);
}

void BidimensionalGrid::steinerRoute( EdgeCostFunction edgeCostFunction ){
    for(Net & n : nets){
        steinerRouteNet(edgeCostFunction, n);
    }
}

} // End namespace spaghetti



