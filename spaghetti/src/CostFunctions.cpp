
#include "spaghetti/CostFunctions.h"

namespace spaghetti{

EdgeEvalFunction edgeDemandFunction(){
    return [](EdgeProperties const & e)->Cost{
        return e.demand;
    };
}
EdgeEvalFunction edgeOverflowFunction(){
    return [](EdgeProperties const & e)->Cost{
        return std::max((Cost) e.demand-e.capacity, 0.0f);
    };
}
EdgeEvalFunction edgeRatioFunction(){
    return [](EdgeProperties const & e)->Cost{
        if(e.demand==0)
            return e.demand;
        else
            return ((Cost) e.demand) / ((Cost) e.capacity);
    };
}

EdgeEvalFunction basicEdgeEvalFunction(){
    return [](EdgeProperties const & e)->Cost{
        return 0.0f;
    };
}
EdgeEvalFunction linearEdgeEvalFunction(Cost penalty){
    return [penalty](EdgeProperties const & e)->Cost{
        return e.basicCost * penalty * e.demand * e.demand;
    };
}
EdgeEvalFunction thresholdEdgeEvalFunction(Cost multiplier, float threshold){
    return [multiplier, threshold](EdgeProperties const & e)->Cost{
        float t = threshold * e.capacity;
        float dem = std::min(t, (Cost) e.demand);
        float overflow = e.demand - dem;
        return e.basicCost * overflow * multiplier;
    };
}
EdgeEvalFunction dualthresholdEdgeEvalFunction(Cost multiplier, float t1, float t2){
    return [multiplier, t1, t2](EdgeProperties const & e)->Cost{
        float c1 = t1 * e.capacity, c2 = t2 * e.capacity;
        float partialOverflow = std::min(c2 - c1, std::max(e.demand - c1, 0.0f));
        float overflow = std::max(e.demand - c2, 0.0f);
        return e.basicCost * (partialOverflow * 0.5f + overflow) * multiplier;
    };
}

EdgeCostFunction basicEdgeCostFunction(){
    return getCostFunction(basicEdgeEvalFunction());
}
EdgeCostFunction linearEdgeCostFunction(Cost penalty){
    return getCostFunction(linearEdgeEvalFunction(penalty));
}
EdgeCostFunction thresholdEdgeCostFunction(Cost multiplier, float threshold){
    return getCostFunction(thresholdEdgeEvalFunction(multiplier, threshold));
}
EdgeCostFunction dualthresholdEdgeCostFunction(Cost multiplier, float t1, float t2){
    return getCostFunction(dualthresholdEdgeEvalFunction(multiplier, t1, t2));
}

EdgePredicate overflowPredicate(float threshold){
    return [threshold](EdgeProperties const & e)->bool{
        return e.demand > e.capacity * threshold;
    };
}

} // End namespace spaghetti

