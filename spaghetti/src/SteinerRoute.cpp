

#include "coloquinte/circuit_helper.hxx"
#include "spaghetti/Grid.h"

namespace spaghetti{

void BidimensionalGrid::steinerRouteNet ( EdgeCostFunction edgeCostFunction, Net & n ){
    using namespace coloquinte;

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
    typedef std::pair<unsigned, bool    > CIPair;
    typedef std::pair<unsigned, unsigned> CCPair;
    std::vector<std::vector<CIPair> > inboundVertices(points.size()); // For each pin, which other pins connect to it according to the horizontal topology
    std::vector<CCPair> minmaxs(points.size());
    for(index_t i=0; i<points.size(); ++i){
        inboundVertices[i].emplace_back(points[i].y_, true);
        minmaxs[i] = CCPair(points[i].y_, points[i].y_);
    }
    for(auto const e : topology){
        if(minmaxs[e.second].first > minmaxs[e.first].second){
            inboundVertices[e.second].emplace_back(minmaxs[e.first].second, false);
            inboundVertices[e.first].emplace_back(minmaxs[e.first].second, true); // Don't have the right to remove it since it Z-routes to the next component
            minmaxs[e.second].first = minmaxs[e.first].second;
        }
        if(minmaxs[e.second].second < minmaxs[e.first].first){
            inboundVertices[e.second].emplace_back(minmaxs[e.first].first, false);
            inboundVertices[e.first].emplace_back(minmaxs[e.first].first, true); // Don't have the right to remove it since it Z-routes to the next component
            minmaxs[e.second].second = minmaxs[e.first].first;
        }
    }
    for(auto & vlist : inboundVertices)
        std::sort(vlist.begin(), vlist.end(), [](CIPair a, CIPair b)->bool{return a.first < b.first; });

    // Get which parts may be routed using a different Z-route
    std::vector<CCPair> mandatoryMinMaxs(points.size());
    for(index_t i=0; i<points.size(); ++i){
        //mandatoryMinMaxs[i].first  = inboundVertices[i][0].second     ? inboundVertices[i][0].first : inboundVertices[i][1].first;
        //mandatoryMinMaxs[i].second = inboundVertices[i].back().second ? inboundVertices[i].back().first : inboundVertices[i][inboundVertices[i].size()-2].first;
        mandatoryMinMaxs[i] = minmaxs[i];
    }

    // Second, materialize those edges
    for(index_t i=0; i<points.size(); ++i){
        for(unsigned y=mandatoryMinMaxs[i].first; y<mandatoryMinMaxs[i].second; ++y)
            routeEdge(getVerticalEdgeIndex(points[i].x_, y), n);
        routeEdge(getTurnEdgeIndex(points[i].x_, points[i].y_), n);
    }

    // Third, route what remains using horizontal wires or Z-routes
    for(auto const edge : topology){ // Route from component edge.first to component edge.second
        unsigned x1 = points[edge.first ].x_;
        unsigned x2 = points[edge.second].x_;
        auto mm1 = mandatoryMinMaxs[edge.first ];
        auto mm2 = mandatoryMinMaxs[edge.second];
        unsigned xmin=std::min(x1, x2), xmax=std::max(x1, x2);

        // Now route between the vertical lines (x1, mm1) to (x2, mm2)
        if(mm1.first <= mm2.second and mm2.first <= mm1.second){ // Overlap on y: route an horizontal wire
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
                routeEdge( getHorizontalEdgeIndex(x, bestY), n);
            routeEdge( getTurnEdgeIndex(xmin, bestY), n);
            routeEdge( getTurnEdgeIndex(xmax, bestY), n);
        }
        else{ // No overlap: Z-route
            Cost minCost = std::numeric_limits<Cost>::infinity();
            unsigned bestX = std::numeric_limits<unsigned>::max();
            // Find the two points we need to connect
            unsigned ymin=std::min(mm1.second, mm2.second), ymax=std::max(mm1.first, mm2.first);
            // What is the y corresponding to xmin/xmax?
            unsigned mm1y = mm1.first == ymax ? mm1.first : mm1.second;
            unsigned mm2y = mm2.first == ymax ? mm2.first : mm2.second;
            unsigned cymin = xmin == x1 ? mm1y : mm2y;
            unsigned cymax = xmax == x1 ? mm1y : mm2y;
            for(unsigned turnx=xmin; turnx<=xmax; ++turnx){
                // TODO: verify the turn edges at the boundaries of the Z-route; create them if they are not router yet
                Cost totCost = edgeCostFunction(getTurnEdge(turnx, ymin), n) + edgeCostFunction(getTurnEdge(turnx, ymax), n);
                for(unsigned y=ymin; y<ymax; ++y)
                    totCost += edgeCostFunction(getVerticalEdge(turnx, y), n);
                for(unsigned x=xmin; x<turnx; ++x)
                    totCost += edgeCostFunction(getHorizontalEdge(x, cymin), n);
                for(unsigned x=turnx; x<xmax; ++x)
                    totCost += edgeCostFunction(getHorizontalEdge(x, cymax), n);
                if(totCost < minCost){
                    bestX = turnx;
                    minCost = totCost;
                }
            }
            assert(bestX < xdim and std::isfinite(minCost));
            for(unsigned x=std::min(x1, x2); x<std::max(x1, x2); ++x){
                if(bestX != xmin)
                    routeEdge(getTurnEdgeIndex(bestX, cymin), n);
                if(bestX != xmax)
                    routeEdge(getTurnEdgeIndex(bestX, cymax), n);
                routeEdge(getTurnEdgeIndex(xmin, cymin), n);
                routeEdge(getTurnEdgeIndex(xmax, cymax), n);
                for(unsigned y=ymin; y<ymax; ++y)
                    routeEdge(getVerticalEdgeIndex(bestX, y), n);
                for(unsigned x=xmin; x<bestX; ++x)
                    routeEdge(getHorizontalEdgeIndex(x, cymin), n);
                for(unsigned x=bestX; x<xmax; ++x)
                    routeEdge(getHorizontalEdgeIndex(x, cymax), n);
            }
        }
    }

    // Third, potentially cleanup the routing: remove edges redundancy due to non-ponctual components
    // TODO
    unrouteUnusedEdges(n);
}

void BidimensionalGrid::steinerRoute( EdgeCostFunction edgeCostFunction ){
    for(Net & n : nets){
        steinerRouteNet(edgeCostFunction, n);
    }
}

} // End namespace spaghetti



