// ****************************************************************************************************
// File: RdsUnit.h
// Authors: Wu YiFei
// Date   : 21/12/2006 
// ****************************************************************************************************

#ifndef HURRICANE_RDSUNIT
#define HURRICANE_RDSUNIT

#include <math.h>

//BEGIN_NAMESPACE_HURRICANE


namespace Hurricane {



// ****************************************************************************************************
// Utilitarians 
// ****************************************************************************************************

extern const long& getRdsUnit();

extern const long& getRdsPhysicalGrid();

extern const long& getRdsLambda();  

//  ------------------------------------------------------------------------
//  Function : "ConvertRealToRdsUnit(const double&)".
//  

  /*  \inline long ConvertRealToRdsUnit(const double& value)
   *  \param  value    the value en Micro to convert to RdsUnit.
   *
   *  Get and return the value en RdsUnit.  
   *
   */

inline long ConvertRealToRdsUnit(const double& value)
// *******************************************************
{
   long tmp_value =  (long)rint(value*getRdsUnit());
   return (tmp_value%2)==0?tmp_value:(tmp_value+1);
}  


//END_NAMESPACE_HURRICANE

}

#endif // HURRICANE_RDSUNIT


