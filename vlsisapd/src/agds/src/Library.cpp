#include <iostream>
#include <fstream>
#include <ctime>
#include <cmath>
#include <cassert>
#include <algorithm>
using namespace std;

#include "vlsisapd/agds/Library.h"
#include "vlsisapd/agds/Structure.h"
#include "vlsisapd/agds/GdsUtils.h"

namespace AGDS {

Library::Library(string libName) 
    : _libName(libName)
    , _userUnits(0.00)
    , _physUnits(0.00) {}


bool Library::addStructure(Structure* gdsStruct) {
    if(gdsStruct)
        _structs.push_back(gdsStruct);
    else {
        cerr << "[GDS DRIVE ERROR]: cannot hold GdsStructure: "  << gdsStruct->getName() << endl;
        return false;
    }
    return true;
}

void writeHeader(std::ostream &s) {
  std::vector<std::int16_t> version = {5};
  writeRecord(s, Header, version);
}

void writeBeginLib(std::ostream &s) {
  writeTime(s, BgnLib);
}

void writeLibName(std::ostream &s, const std::string &name) {
  writeRecord(s, LibName, name);
}

void writeUnits(std::ostream &s, double user, double phys) {
  std::vector<double> units = {user, phys};
  writeRecord(s, Units, units);
}

void writeEndLib(std::ostream &s) {
    writeRecord(s, EndLib);
}

bool Library::writeGDS   (string filename) {
    ofstream file(filename.c_str());
    writeHeader(file);
    writeBeginLib(file);
    writeLibName(file, _libName);
    writeUnits(file, _userUnits, _physUnits);

    for ( vector<Structure*>::iterator it = _structs.begin() ; it < _structs.end() ; it++ ) {
        (*it)->writeGDS(file);
    }

    writeEndLib(file);

    return true;
}

bool Library::writeAGDS  (string filename) {
    ofstream file(filename.c_str());
    time_t curtime = time(0);
    tm now = *localtime(&curtime);
    char date[BUFSIZ]={0};
    const char format[]="%y-%m-%d  %H:%M:%S";
    if (strftime(date, sizeof(date)-1, format, &now) == 0)
          cerr << "[GDS DRIVE ERROR]: cannot build current date." << endl;
    
    // Header
    file << "HEADER 5;" << endl
         << "BGNLIB;" << endl
         << "  LASTMOD {" << date << "};" << endl
         << "  LASTACC {" << date << "};" << endl
         << "LIBNAME " << _libName << ";" << endl
         << "UNITS;" << endl
         << "  USERUNITS " << _userUnits << ";" << endl;
    file << scientific << "  PHYSUNITS " << _physUnits << ";" << endl
           << endl;
    file.unsetf(ios::floatfield);

    for ( vector<Structure*>::iterator it = _structs.begin() ; it < _structs.end() ; it++ ) {
        (*it)->writeAGDS(file);
    }

    // Footer
    file << "ENDLIB;" << endl;


    return true;
}

} // namespace
