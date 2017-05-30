#include <iostream>
#include <iomanip>
#include <vector>
using namespace std;

#include "vlsisapd/agds/Rectangle.h"
#include "vlsisapd/agds/GdsUtils.h"

namespace AGDS {
Element::~Element () { }

Rectangle::Rectangle(int layer, int xmin, int ymin, int xmax, int ymax) 
    : Element(layer)
    , _xmin(xmin)
    , _ymin(ymin)
    , _xmax(xmax)
    , _ymax(ymax) {}

void Rectangle::writeAGDS (ofstream &file) {
    file << "BOUNDARY;" << endl
         << "LAYER " << _layer << ";" << endl
         << "DATATYPE 0;" << endl
         << "XY 5;" << endl
         << "  X: " << _xmin << ";\tY: " << _ymin << ";" << endl
         << "  X: " << _xmin << ";\tY: " << _ymax << ";" << endl
         << "  X: " << _xmax << ";\tY: " << _ymax << ";" << endl
         << "  X: " << _xmax << ";\tY: " << _ymin << ";" << endl
         << "  X: " << _xmin << ";\tY: " << _ymin << ";" << endl
         << "ENDEL;" << endl
         << endl;
}

void writeLayer(ofstream &file, int16_t n) {
  vector<int16_t> d = {n};
  writeRecord(file, Layer, d);
}

void writeDatatype(ofstream &file) {
  vector<int16_t> d = {0};
  writeRecord(file, Datatype, d);
}

void Rectangle::writeGDS (ofstream &file) {
  writeRecord(file, Boundary);
  writeLayer(file, _layer);
  writeDatatype(file);
  vector<int32_t> coords = {
      _xmin, _ymin
    , _xmin, _ymax
    , _xmax, _ymax
    , _xmax, _ymin
    , _xmin, _ymin
  };
  writeRecord(file, XY, coords);
  writeRecord(file, Endl);
}

} // namespace
