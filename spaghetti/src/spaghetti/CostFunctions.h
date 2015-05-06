
#ifndef SPAGHETTI_COST_FCTS
#define SPAGHETTI_COST_FCTS

#include "Graph.h"

namespace spaghetti{

inline EdgeCostFunction getCostFunction(EdgeEvalFunction const & fct){
    return [fct](EdgeProperties const & e, NetProperties const & n)->Cost{
        EdgeProperties tmp = e;
        tmp.demand += n.demand;
        return fct(tmp) - fct(e) + n.cost * e.basicCost + n.demand * e.historyCost;
    };
}
inline VertexCostFunction getCostFunction(VertexEvalFunction const & fct){
    return [fct](VertexProperties const & v, NetProperties const & n)->Cost{
        return n.cost * v.basicCost + n.demand * v.historyCost;
    };
}

EdgeEvalFunction edgeDemandFunction();
EdgeEvalFunction edgeOverflowFunction();
EdgeEvalFunction edgeRatioFunction();

EdgeEvalFunction basicEdgeEvalFunction();
EdgeEvalFunction linearEdgeEvalFunction(Cost penalty);
EdgeEvalFunction thresholdEdgeEvalFunction(Cost multiplier, float threshold=1.0f);
EdgeEvalFunction dualthresholdEdgeEvalFunction(Cost multiplier, float t1=0.8f, float t2=1.0f);

EdgeCostFunction basicEdgeCostFunction();
EdgeCostFunction linearEdgeCostFunction(Cost penalty);
EdgeCostFunction thresholdEdgeCostFunction(Cost multiplier, float threshold=1.0f);
EdgeCostFunction dualthresholdEdgeCostFunction(Cost multiplier, float t1=0.8f, float t2=1.0f);

EdgePredicate    overflowPredicate(float threshold=1.0f);

} // End namespace spaghetti

#endif

