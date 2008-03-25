// ****************************************************************************************************
// File: Point.cpp
// Authors: R. Escassut
// Copyright (c) BULL S.A. 2000-2004, All Rights Reserved
// ****************************************************************************************************
// 21-10-2003 Alignment BULL-LIP6 : added operators+ += ... (questionnable!)

#include "Point.h"

namespace Hurricane {



// ****************************************************************************************************
// Point implementation
// ****************************************************************************************************

Point::Point()
// ***********
:    _x(0),
    _y(0)
{
}

Point::Point(const Unit& x, const Unit& y)
// ***************************************
:    _x(x),
    _y(y)
{
}

Point::Point(const Point& point)
// *****************************
:    _x(point._x),
    _y(point._y)
{
}

Point& Point::operator=(const Point& point)
// ****************************************
{
    _x = point._x;
    _y = point._y;
    return *this;
}

bool Point::operator==(const Point& point) const
// *********************************************
{
    return ((_x == point._x) && (_y == point._y));
}

bool Point::operator!=(const Point& point) const
// *********************************************
{
    return ((_x != point._x) || (_y != point._y));
}

Point Point::operator+(const Point& point) const
// *********************************************
{
    return Point(_x+point._x,_y+point._y);
}

Point Point::operator-(const Point& point) const
// *********************************************
{
    return Point(_x-point._x,_y-point._y);
}

Point& Point::operator+=(const Point &point)
// *****************************************
{
    _x += point._x;
    _y += point._y;
    return *this;
}

Point& Point::operator-=(const Point &point)
// *****************************************
{
    _x -= point._x;
    _y -= point._y;
    return *this;
}

Point& Point::translate(const Unit& dx, const Unit& dy)
// ****************************************************
{
    _x += dx;
    _y += dy;
    return *this;
}

Point Point::getTranslated(const Unit& dx, const Unit& dy) const {
    return Point(*this).translate(dx, dy);
}

string Point::_getString() const
// *****************************
{
    return "<" + _TName("Point") + " " + getValueString(_x) + " " + getValueString(_y) + ">";
}

Record* Point::_getRecord() const
// ******************************
{
    Record* record = new Record(getString(this));
    record->Add(getSlot("X", &_x));
    record->Add(getSlot("Y", &_y));
    return record;
}



} // End of Hurricane namespace.

// ****************************************************************************************************
// Copyright (c) BULL S.A. 2000-2004, All Rights Reserved
// ****************************************************************************************************