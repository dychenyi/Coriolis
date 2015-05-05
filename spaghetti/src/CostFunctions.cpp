
#include "spaghetti/CostFunctions.h"

namespace spaghetti{

EdgeEvalFunction basicEdgeEvalFunction(){
    return [](EdgeProperties const & e)->Cost{
        return e.demand * e.basicCost;
    };
}
EdgeEvalFunction linearEdgeEvalFunction(Cost penalty){
    return [penalty](EdgeProperties const & e)->Cost{
        return e.basicCost * e.demand * (1.0f + penalty * e.demand);
    };
}
EdgeEvalFunction thresholdEdgeEvalFunction(Cost multiplier, float threshold){
    return [multiplier, threshold](EdgeProperties const & e)->Cost{
        float t = threshold * e.capacity;
        float dem = std::min(t, (Cost) e.demand);
        float overflow = e.demand - dem;
        return e.basicCost * (dem + overflow * multiplier);
    };
}
EdgeEvalFunction dualthresholdEdgeEvalFunction(Cost multiplier, float t1, float t2){
    return [multiplier, t1, t2](EdgeProperties const & e)->Cost{
        float c1 = t1 * e.capacity;
        float c2 = t2 * e.capacity;
        float d1 = std::min(c1, (Cost) e.demand);
        float d2 = std::min(c2-c1, e.demand-d1);
        float overflow = e.demand - d2;
        // Quadratic between t1 and t2, derivative from 1 to multiplier
        // TODO: remove divide
        Cost quadfunc = c2 > c1 ? d2*(1.0f + d2*(multiplier-1.0f)/(c2-c1)) : 0.0;
        return e.basicCost * (d1 + quadfunc + overflow * multiplier);
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

