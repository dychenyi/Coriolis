#ifndef __GDS_GDSUTILS_H
#define __GDS_GDSUTILS_H

#include <cstdint>
#include <iosfwd>
#include <vector>

namespace AGDS {

enum RecordType : std::uint8_t {
  Header = 0
, BgnLib = 1
, LibName = 2
, Units = 3
, EndLib = 4
, BgnStr = 5
, StrName = 6
, EndStr = 7
, Boundary = 8
, Layer = 13
, Datatype = 14
, XY = 16
, Endl = 17
};

enum DataType : std::uint8_t {
  None = 0
, BitArray = 1
, S16 = 2
, S32 = 3
, F32 = 4
, F64 = 5
, Str = 6
};

void writeRecord(std::ostream &s, RecordType rType, DataType dType=None, const char *data=nullptr, int len=0);
void writeRecord(std::ostream &s, RecordType type, const std::string &d);
void writeRecord(std::ostream &s, RecordType type, const std::vector<int16_t> &d);
void writeRecord(std::ostream &s, RecordType type, const std::vector<int32_t> &d);
void writeRecord(std::ostream &s, RecordType type, const std::vector<double> &d);
void writeTime(std::ostream &s, RecordType type);

}
#endif

