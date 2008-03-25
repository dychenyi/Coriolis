// ****************************************************************************************************
// File: Slice.cpp
// Authors: R. Escassut
// Copyright (c) BULL S.A. 2000-2004, All Rights Reserved
// ****************************************************************************************************

#include "Slice.h"
#include "Cell.h"
#include "Component.h"
#include "Marker.h"
#include "Layer.h"
#include "Error.h"

namespace Hurricane {



// ****************************************************************************************************
// Slice implementation
// ****************************************************************************************************

Slice::Slice(Cell* cell, Layer* layer)
// ***********************************
:    _cell(cell),
    _layer(layer),
    _quadTree(),
    _nextOfCellSliceMap(NULL)
{
    if (!_cell)
        throw Error("Can't create " + _TName("Slice") + " : null cell");

    if (!_layer)
        throw Error("Can't create " + _TName("Slice") + " : null layer");

    if (_cell->getSlice(_layer))
        throw Error("Can't create " + _TName("Slice") + " : already exists");

    _cell->_getSliceMap()._Insert(this);
}

Slice::~Slice()
// ************
{
    _cell->_getSliceMap()._Remove(this);
}

Components Slice::getComponents() const
// ************************************
{
    // return _quadTree.getGos().getSubSet<Component*>();
    return SubTypeCollection<Go*, Component*>(_quadTree.getGos());
}

Components Slice::getComponentsUnder(const Box& area) const
// ********************************************************
{
    // return _quadTree.getGosUnder(area).getSubSet<Component*>();
    return SubTypeCollection<Go*, Component*>(_quadTree.getGosUnder(area));
}

Markers Slice::getMarkers() const
// ******************************
{
    // return _quadTree.getGos().getSubSet<Marker*>();
    return SubTypeCollection<Go*, Marker*>(_quadTree.getGos());
}

Markers Slice::getMarkersUnder(const Box& area) const
// **************************************************
{
    // return _quadTree.getGosUnder(area).getSubSet<Marker*>();
    return SubTypeCollection<Go*, Marker*>(_quadTree.getGosUnder(area));
}

Slice* Slice::_create(Cell* cell, Layer* layer)
// ********************************************
{
    Slice* slice = new Slice(cell, layer);

    return slice;
}

void Slice::_destroy()
// ******************
{
    delete this;
}

string Slice::_getString() const
// *****************************
{
    string s = "<" + _TName("Slice");
    // s += " " + getString(_cell->getName());
    s += " " + getString(_layer->getName());
    s += ">";
    return s;
}

Record* Slice::_getRecord() const
// ************************
{
     Record* record = new Record(getString(this));
    if (record) {
        record->Add(getSlot("Cell", _cell));
        record->Add(getSlot("Layer", _layer));
        record->Add(getSlot("QuadTree", &_quadTree));
    }
    return record;
}

} // End of Hurricane namespace.

// ****************************************************************************************************
// Copyright (c) BULL S.A. 2000-2004, All Rights Reserved
// ****************************************************************************************************