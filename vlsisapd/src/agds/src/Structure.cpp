#include <iostream>
#include <ctime>
using namespace std;

#include "vlsisapd/agds/Structure.h"
#include "vlsisapd/agds/Element.h"
#include "vlsisapd/agds/GdsUtils.h"

namespace AGDS {
Structure::Structure(string strName) 
    : _strName(strName) {}


bool Structure::addElement(Element* gdsElement) {
    if(gdsElement)
        _elements.emplace_back(gdsElement);
    else {
        cerr << "[GDS DRIVE ERROR]: cannot hold Element." << endl;
        return false;
    }
    return true;
}

void Structure::writeAGDS(ofstream &file) {
    time_t curtime = time(0);
    tm now = *localtime(&curtime);
    char date[BUFSIZ]={0};
    const char format[]="%y-%m-%d  %H:%M:%S";
    if (strftime(date, sizeof(date)-1, format, &now) == 0)
        cerr << "[GDS DRIVE ERROR]: cannot build current date." << endl;

    // Header
    file << "BGNSTR;" << endl
         << "  CREATION {" << date << "};" << endl
         << "  LASTMOD  {" << date << "};" << endl
         << "STRNAME " << _strName << ";" << endl
         << endl;

    // For each Element : write element.
    for ( auto & elt : _elements ) {
        elt->writeAGDS(file);
    }

    // Footer
    file << "ENDSTR;" << endl;
}

void writeBeginStruct(std::ostream &s) {
  writeTime(s, BgnStr);
}

void writeStructName(std::ostream &s, const std::string &name) {
    writeRecord(s, StrName, name);
}

void writeEndStruct(std::ostream &s) {
  writeRecord(s, EndStr);
}

void Structure::writeGDS(ofstream &file) {
  writeBeginStruct(file);
  writeStructName(file, _strName);
  for ( auto & elt : _elements ) {
      elt->writeGDS(file);
  }
  writeEndStruct(file);
}

} // namespace
