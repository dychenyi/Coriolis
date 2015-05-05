
#ifndef SPAGHETTI_COST_FCTS
#define SPAGHETTI_COST_FCTS

#include "Graph.h"

namespace spaghetti{

inline EdgeCostFunction getCostFunction(EdgeEvalFunction const & fct){
    return [fct](EdgeProperties const & e, Capacity d)->Cost{
        EdgeProperties tmp = e;
        tmp.demand += d;
        return fct(tmp) - fct(e);
    };
}
inline VertexCostFunction getCostFunction(VertexEvalFunction const & fct){
    return [fct](VertexProperties const & e, Capacity d)->Cost{
        VertexProperties tmp = e;
        return fct(tmp) - fct(e);
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

