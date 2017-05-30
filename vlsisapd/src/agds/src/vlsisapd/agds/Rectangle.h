#ifndef __GDS_RECTANGLE_H
#define __GDS_RECTANGLE_H

#include <fstream>

#include "vlsisapd/agds/Element.h"

namespace AGDS {
class Rectangle : public Element {
    public:
        Rectangle (int layer, int xmin, int ymin, int xmax, int ymax);
        void  writeAGDS (std::ofstream &file);
        void  writeGDS  (std::ofstream &file);

    private:
        int _xmin;
        int _ymin;
        int _xmax;
        int _ymax;
};
}
#endif

