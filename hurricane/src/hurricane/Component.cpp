// ****************************************************************************************************
// File: Component.cpp
// Authors: R. Escassut
// Copyright (c) BULL S.A. 2000-2004, All Rights Reserved
// ****************************************************************************************************

#include "Component.h"
#include "Net.h"
#include "Cell.h"
#include "Rubber.h"
#include "Slice.h"
#include "BasicLayer.h"
#include "Error.h"

namespace Hurricane {



// ****************************************************************************************************
// Filters declaration & implementation
// ****************************************************************************************************

class Component_IsUnderFilter : public Filter<Component*> {
// ******************************************************

    public: Box _area;

    public: Component_IsUnderFilter(const Box& area)
    // *********************************************
    :    _area(area)
    {
    };

    public: Component_IsUnderFilter(const Component_IsUnderFilter& filter)
    // *******************************************************************
    :    _area(filter._area)
    {
    };

    public: Component_IsUnderFilter& operator=(const Component_IsUnderFilter& filter)
    // ******************************************************************************
    {
        _area = filter._area;
        return *this;
    };

    public: virtual Filter<Component*>* getClone() const
    // *************************************************
    {
        return new Component_IsUnderFilter(*this);
    };

    public: virtual bool accept(Component* component) const
    // ****************************************************
    {
        return _area.intersect(component->getBoundingBox());
    };

    public: virtual string _getString() const
    // **************************************
    {
        return "<" + _TName("Component::IsUnderFilter") + " " + getString(_area) + ">";
    };

};



// ****************************************************************************************************
// Component_Hooks declaration
// ****************************************************************************************************

class Component_Hooks : public Collection<Hook*> {
// *********************************************

// Types
// *****

    public: typedef Collection<Hook*> Inherit;

    public: class Locator : public Hurricane::Locator<Hook*> {
    // *****************************************************

        public: typedef Hurricane::Locator<Hook*> Inherit;

        private: const Component* _component;
        private: Hook* _hook;

        public: Locator(const Component* component = NULL);
        public: Locator(const Locator& locator);

        public: Locator& operator=(const Locator& locator);

        public: virtual Hook* getElement() const;
        public: virtual Hurricane::Locator<Hook*>* getClone() const;

        public: virtual bool isValid() const;

        public: virtual void progress();

        public: virtual string _getString() const;

    };

// Attributes
// **********

    private: const Component* _component;

// Constructors
// ************

    public: Component_Hooks(const Component* component = NULL);
    public: Component_Hooks(const Component_Hooks& hooks);

// Operators
// *********

    public: Component_Hooks& operator=(const Component_Hooks& hooks);

// Accessors
// *********

    public: virtual Collection<Hook*>* getClone() const;
    public: virtual Hurricane::Locator<Hook*>* getLocator() const;

// Others
// ******

    public: virtual string _getString() const;

};



// ****************************************************************************************************
// Component_ConnexComponents declaration
// ****************************************************************************************************

class Component_ConnexComponents : public Collection<Component*> {
// *************************************************************

// Types
// *****

    public: typedef Collection<Component*> Inherit;

    public: class Locator : public Hurricane::Locator<Component*> {
    // **********************************************************

        public: typedef Hurricane::Locator<Component*> Inherit;

        private: const Component* _component;
        private: set<Component*> _componentSet;
        private: stack<Component*> _componentStack;

        public: Locator(const Component* component = NULL);
        public: Locator(const Locator& locator);

        public: Locator& operator=(const Locator& locator);

        public: virtual Component* getElement() const;
        public: virtual Hurricane::Locator<Component*>* getClone() const;

        public: virtual bool isValid() const;

        public: virtual void progress();

        public: virtual string _getString() const;

    };

// Attributes
// **********

    private: const Component* _component;

// Constructors
// ************

    public: Component_ConnexComponents(const Component* component = NULL);
    public: Component_ConnexComponents(const Component_ConnexComponents& connexComponents);

// Operators
// *********

    public: Component_ConnexComponents& operator=(const Component_ConnexComponents& connexComponents);

// Accessors
// *********

    public: virtual Collection<Component*>* getClone() const;
    public: virtual Hurricane::Locator<Component*>* getLocator() const;

// Others
// ******

    public: virtual string _getString() const;

};



// ****************************************************************************************************
// Component_SlaveComponents declaration
// ****************************************************************************************************

class Component_SlaveComponents : public Collection<Component*> {
// ************************************************************

// Types
// *****

    public: typedef Collection<Component*> Inherit;

    public: class Locator : public Hurricane::Locator<Component*> {
    // **********************************************************

        public: typedef Hurricane::Locator<Component*> Inherit;

        private: const Component* _component;
        private: set<Component*> _componentSet;
        private: stack<Component*> _componentStack;

        public: Locator(const Component* component = NULL);
        public: Locator(const Locator& locator);

        public: Locator& operator=(const Locator& locator);

        public: virtual Component* getElement() const;
        public: virtual Hurricane::Locator<Component*>* getClone() const;

        public: virtual bool isValid() const;

        public: virtual void progress();

        public: virtual string _getString() const;

};

// Attributes
// **********

    private: const Component* _component;

// Constructors
// ************

    public: Component_SlaveComponents(const Component* component = NULL);
    public: Component_SlaveComponents(const Component_SlaveComponents& slaveComponents);

// Operators
// *********

    public: Component_SlaveComponents& operator=(const Component_SlaveComponents& slaveComponents);

// Accessors
// *********

    public: virtual Collection<Component*>* getClone() const;
    public: virtual Hurricane::Locator<Component*>* getLocator() const;

// Others
// ******

    public: virtual string _getString() const;
};



// ****************************************************************************************************
// Component implementation
// ****************************************************************************************************

Component::Component(Net* net, bool inPlugCreate)
// **********************************************
:    Inherit(),
    _net(net),
    _rubber(NULL),
    _bodyHook(this),
    _nextOfNetComponentSet(NULL)
{
    if (!inPlugCreate && !_net)
        throw Error("Can't create " + _TName("Component") + " : null net");
}

Cell* Component::getCell() const
// *****************************
{
    return _net->getCell();
}

Hooks Component::getHooks() const
// ******************************
{
    return Component_Hooks(this);
}

Components Component::getConnexComponents() const
// **********************************************
{
    return Component_ConnexComponents(this);
}

Components Component::getSlaveComponents() const
// *********************************************
{
    return Component_SlaveComponents(this);
}

ComponentFilter Component::getIsUnderFilter(const Box& area)
// *********************************************************
{
    return Component_IsUnderFilter(area);
}

void Component::Materialize()
// **************************
{
// trace << "Materialize() - " << this << endl;

    if (!IsMaterialized()) {
        Cell*  cell  = getCell();
        Layer* layer = getLayer();
        if (cell && layer) {
            Slice* slice = cell->getSlice(layer);
            if (!slice) slice = Slice::_create(cell, layer);
            QuadTree* quadTree = slice->_getQuadTree();
            quadTree->Insert(this);
            cell->_Fit(quadTree->getBoundingBox());
        } else {
          //cerr << "[WARNING] " << this << " not inserted into QuadTree." << endl;
        }
    }
}

void Component::Unmaterialize()
// ****************************
{
// trace << "Unmaterializing " << this << endl;

    if (IsMaterialized()) {
        Cell* cell = getCell();
        Slice* slice = cell->getSlice(getLayer());
        if (slice) {
            cell->_Unfit(getBoundingBox());
            slice->_getQuadTree()->Remove(this);
            if (slice->IsEmpty()) slice->_destroy();
        }
    }
}

void Component::Invalidate(bool propagateFlag)
// *******************************************
{
    Inherit::Invalidate(false);

    if (propagateFlag) {
        Rubber* rubber = getRubber();
        if (rubber) rubber->Invalidate();
        for_each_component(component, getSlaveComponents()) {
            component->Invalidate(false);
            end_for;
        }
    }
}

void Component::_postCreate()
// **************************
{
    if (_net) _net->_getComponentSet()._Insert(this);

    Inherit::_postCreate();
}

void Component::_preDestroy()
// *************************
{
// trace << "entering Component::_Predestroy: " << this << endl;
// trace_in();

    clearProperties();

    set<Component*> componentSet;
    getSlaveComponents().fill(componentSet);

    set<Hook*> masterHookSet;
    componentSet.insert(this);
    for_each_component(component, getCollection(componentSet)) {
        component->Unmaterialize();
        for_each_hook(hook, component->getHooks()) {
            for_each_hook(hook, hook->getHooks()) {
                if (hook->IsMaster() && (componentSet.find(hook->getComponent()) == componentSet.end()))
                    masterHookSet.insert(hook);
                end_for;
            }
            if (!hook->IsMaster()) hook->Detach();
            end_for;
        }
        end_for;
    }

    componentSet.erase(this);
    for_each_component(component, getCollection(componentSet)) {
        component->destroy();
        end_for;
    }

    set<Rubber*> rubberSet;
    set<Hook*> mainMasterHookSet;
    for_each_hook(hook, getCollection(masterHookSet)) {
        Rubber* rubber = hook->getComponent()->getRubber();
        if (!rubber)
            mainMasterHookSet.insert(hook);
        else {
            if (rubberSet.find(rubber) == rubberSet.end()) {
                rubberSet.insert(rubber);
                mainMasterHookSet.insert(hook);
            }
        }
        end_for;
    }
    Hook* masterHook = NULL;
    for_each_hook(hook, getCollection(mainMasterHookSet)) {
        if (!masterHook)
            masterHook = hook;
        else
            hook->Merge(masterHook);
        end_for;
    }
    /**/

    _bodyHook.Detach();

    Inherit::_preDestroy();

    if (_net) _net->_getComponentSet()._Remove(this);


    // trace << "exiting Component::_Predestroy:" << endl;
    // trace_out();
}

string Component::_getString() const
// *********************************
{
    string s = Inherit::_getString();
    if (!_net)
        s.insert(s.length() - 1, " UNCONNECTED");
    else
        s.insert(s.length() - 1, " " + getString(_net->getName()));
    return s;
}

Record* Component::_getRecord() const
// ****************************
{
    Record* record = Inherit::_getRecord();
    if (record) {
        record->Add(getSlot("Net", _net));
        record->Add(getSlot("Rubber", _rubber));
        record->Add(getSlot("BodyHook", &_bodyHook));
    }
    return record;
}

void Component::_SetNet(Net* net)
// ******************************
{
    if (net != _net) {
        if (_net) _net->_getComponentSet()._Remove(this);
        _net = net;
        if (_net) _net->_getComponentSet()._Insert(this);
    }
}

void Component::_SetRubber(Rubber* rubber)
// ***************************************
{
    if (rubber != _rubber) {
        if (_rubber) _rubber->_Release();
        _rubber = rubber;
        if (_rubber) _rubber->_Capture();
    }
}

//bool Component::_IsInterceptedBy(View* view, const Point& point, const Unit& aperture) const
//// *****************************************************************************************
//{
//    Box area(point);
//    area.Inflate(aperture);
//    for_each_basic_layer(basicLayer, getLayer()->getBasicLayers()) {
//        if (view->IsVisible(basicLayer) && getBoundingBox(basicLayer).intersect(area))
//            return true;
//        end_for;
//    }
//
//    return false;
//}
//
//void Component::_Highlight(View* view, const Box& updateArea, const Transformation& transformation)
//// ************************************************************************************************
//{
//    for_each_basic_layer(basicLayer, getLayer()->getBasicLayers()) {
//        _Draw(view, basicLayer, updateArea, transformation);
//        end_for;
//    }
//}
//

// ****************************************************************************************************
// Component::BodyHook implementation
// ****************************************************************************************************

static int BODY_HOOK_OFFSET = -1;

Component::BodyHook::BodyHook(Component* component)
// ************************************************
:    Inherit()
{
    if (!component)
        throw Error("Can't create " + _TName("Component::BodyHook") + " : null component");

    if (BODY_HOOK_OFFSET == -1)
        BODY_HOOK_OFFSET = (unsigned long)this - (unsigned long)component;
}

Component* Component::BodyHook::getComponent() const
// *************************************************
{
    return (Component*)((unsigned long)this - BODY_HOOK_OFFSET);
}

string Component::BodyHook::_getString() const
// *******************************************
{
    return "<" + _TName("Component::BodyHook") + " " + getString(getComponent()) + ">";
}

// ****************************************************************************************************
// Component_Hooks implementation
// ****************************************************************************************************

Component_Hooks::Component_Hooks(const Component* component)
// *********************************************************
:     Inherit(),
    _component(component)
{
}

Component_Hooks::Component_Hooks(const Component_Hooks& hooks)
// ***********************************************************
:     Inherit(),
    _component(hooks._component)
{
}

Component_Hooks& Component_Hooks::operator=(const Component_Hooks& hooks)
// **********************************************************************
{
    _component = hooks._component;
    return *this;
}

Collection<Hook*>* Component_Hooks::getClone() const
// *************************************************
{
    return new Component_Hooks(*this);
}

Locator<Hook*>* Component_Hooks::getLocator() const
// ************************************************
{
    return new Locator(_component);
}

string Component_Hooks::_getString() const
// ***************************************
{
    string s = "<" + _TName("Component::Hooks");
    if (_component) s += " " + getString(_component);
    s += ">";
    return s;
}



// ****************************************************************************************************
// Component_Hooks::Locator implementation
// ****************************************************************************************************

Component_Hooks::Locator::Locator(const Component* component)
// **********************************************************
:    Inherit(),
    _component(component),
    _hook(NULL)
{
    if (_component) _hook = ((Component*)_component)->getBodyHook();
}

Component_Hooks::Locator::Locator(const Locator& locator)
// ******************************************************
:    Inherit(),
    _component(locator._component),
    _hook(locator._hook)
{
}

Component_Hooks::Locator& Component_Hooks::Locator::operator=(const Locator& locator)
// **********************************************************************************
{
    _component = locator._component;
    _hook = locator._hook;
    return *this;
}

Hook* Component_Hooks::Locator::getElement() const
// ***********************************************
{
    return _hook;
}

Locator<Hook*>* Component_Hooks::Locator::getClone() const
// *******************************************************
{
    return new Locator(*this);
}

bool Component_Hooks::Locator::isValid() const
// *******************************************
{
    return (_hook != NULL);
}

void Component_Hooks::Locator::progress()
// **************************************
{
    _hook = NULL;
}

string Component_Hooks::Locator::_getString() const
// ************************************************
{
    string s = "<" + _TName("Component::Hooks::Locator");
    if (_component) s += " " + getString(_component);
    s += ">";
    return s;
}



// ****************************************************************************************************
// Component_ConnexComponents implementation
// ****************************************************************************************************

Component_ConnexComponents::Component_ConnexComponents(const Component* component)
// *******************************************************************************
:     Inherit(),
    _component(component)
{
}

Component_ConnexComponents::Component_ConnexComponents(const Component_ConnexComponents& connexComponents)
// ****************************************************************************************************
:     Inherit(),
    _component(connexComponents._component)
{
}

Component_ConnexComponents&
    Component_ConnexComponents::operator=(const Component_ConnexComponents& connexComponents)
// *****************************************************************************************
{
    _component = connexComponents._component;
    return *this;
}

Collection<Component*>* Component_ConnexComponents::getClone() const
// *****************************************************************
{
    return new Component_ConnexComponents(*this);
}

Locator<Component*>* Component_ConnexComponents::getLocator() const
// ****************************************************************
{
    return new Locator(_component);
}

string Component_ConnexComponents::_getString() const
// **************************************************
{
    string s = "<" + _TName("Component::ConnexComponents");
    if (_component) s += " " + getString(_component);
    s += ">";
    return s;
}



// ****************************************************************************************************
// Component_ConnexComponents::Locator implementation
// ****************************************************************************************************

Component_ConnexComponents::Locator::Locator(const Component* component)
// *********************************************************************
:    Inherit(),
    _component(component),
    _componentSet(),
    _componentStack()
{
    if (_component) {
        _componentSet.insert((Component*)_component);
        _componentStack.push((Component*)_component);
    }
}

Component_ConnexComponents::Locator::Locator(const Locator& locator)
// *****************************************************************
:    Inherit(),
    _component(locator._component),
    _componentSet(locator._componentSet),
    _componentStack(locator._componentStack)
{
}

Component_ConnexComponents::Locator& Component_ConnexComponents::Locator::operator=(const Locator& locator)
// ****************************************************************************************************
{
    _component = locator._component;
    _componentSet = locator._componentSet;
    _componentStack = locator._componentStack;
    return *this;
}

Component* Component_ConnexComponents::Locator::getElement() const
// ***************************************************************
{
    return _componentStack.top();
}

Locator<Component*>* Component_ConnexComponents::Locator::getClone() const
// ***********************************************************************
{
    return new Locator(*this);
}

bool Component_ConnexComponents::Locator::isValid() const
// ******************************************************
{
    return !_componentStack.empty();
}

void Component_ConnexComponents::Locator::progress()
// *************************************************
{
    if (!_componentStack.empty()) {
        Component* component = _componentStack.top();
        _componentStack.pop();
        for_each_hook(componentHook, component->getHooks()) {
            Hook* masterHook = componentHook->getMasterHook();
            if (masterHook) {
                for_each_hook(slaveHook, masterHook->getSlaveHooks()) {
                    Component* component = slaveHook->getComponent();
                    if (_componentSet.find(component) == _componentSet.end()) {
                        _componentSet.insert(component);
                        _componentStack.push(component);
                    }
                    end_for;
                }
                Component* component = masterHook->getComponent();
                if (_componentSet.find(component) == _componentSet.end()) {
                    _componentSet.insert(component);
                    _componentStack.push(component);
                }
            }
            end_for;
        }
    }
}

string Component_ConnexComponents::Locator::_getString() const
// ***********************************************************
{
    string s = "<" + _TName("Component::ConnexComponents::Locator");
    if (_component) s += " " + getString(_component);
    s += ">";
    return s;
}



// ****************************************************************************************************
// Component_SlaveComponents implementation
// ****************************************************************************************************

Component_SlaveComponents::Component_SlaveComponents(const Component* component)
// *****************************************************************************
:     Inherit(),
    _component(component)
{
}

Component_SlaveComponents::Component_SlaveComponents(const Component_SlaveComponents& slaveComponents)
// ***************************************************************************************************
:     Inherit(),
    _component(slaveComponents._component)
{
}

Component_SlaveComponents&
    Component_SlaveComponents::operator=(const Component_SlaveComponents& slaveComponents)
// **************************************************************************************
{
    _component = slaveComponents._component;
    return *this;
}

Collection<Component*>* Component_SlaveComponents::getClone() const
// ****************************************************************
{
    return new Component_SlaveComponents(*this);
}

Locator<Component*>* Component_SlaveComponents::getLocator() const
// ***************************************************************
{
    return new Locator(_component);
}

string Component_SlaveComponents::_getString() const
// *************************************************
{
    string s = "<" + _TName("Component::SlaveComponents");
    if (_component) s += " " + getString(_component);
    s += ">";
    return s;
}



// ****************************************************************************************************
// Component_SlaveComponents::Locator implementation
// ****************************************************************************************************

Component_SlaveComponents::Locator::Locator(const Component* component)
// ********************************************************************
:    Inherit(),
    _component(component),
    _componentSet(),
    _componentStack()
{
    if (_component) {
        _componentSet.insert((Component*)_component);
        Hook* masterHook = ((Component*)_component)->getBodyHook();
        for_each_hook(slaveHook, masterHook->getSlaveHooks()) {
            Component* component = slaveHook->getComponent();
            if (_componentSet.find(component) == _componentSet.end()) {
                _componentSet.insert(component);
                _componentStack.push(component);
            }
            end_for;
        }
    }
}

Component_SlaveComponents::Locator::Locator(const Locator& locator)
// ****************************************************************
:    Inherit(),
    _component(locator._component),
    _componentSet(locator._componentSet),
    _componentStack(locator._componentStack)
{
}

Component_SlaveComponents::Locator&
    Component_SlaveComponents::Locator::operator=(const Locator& locator)
// *********************************************************************
{
    _component = locator._component;
    _componentSet = locator._componentSet;
    _componentStack = locator._componentStack;
    return *this;
}

Component* Component_SlaveComponents::Locator::getElement() const
// **************************************************************
{
    return _componentStack.top();
}

Locator<Component*>* Component_SlaveComponents::Locator::getClone() const
// **********************************************************************
{
    return new Locator(*this);
}

bool Component_SlaveComponents::Locator::isValid() const
// *****************************************************
{
    return !_componentStack.empty();
}

void Component_SlaveComponents::Locator::progress()
// ************************************************
{
    if (!_componentStack.empty()) {
        Component* component = _componentStack.top();
        _componentStack.pop();
        Hook* masterHook = component->getBodyHook();
        for_each_hook(slaveHook, masterHook->getSlaveHooks()) {
            Component* component = slaveHook->getComponent();
            if (_componentSet.find(component) == _componentSet.end()) {
                _componentSet.insert(component);
                _componentStack.push(component);
            }
            end_for;
        }
    }
}

string Component_SlaveComponents::Locator::_getString() const
// **********************************************************
{
    string s = "<" + _TName("Component::SlaveComponents::Locator");
    if (_component) s += " " + getString(_component);
    s += ">";
    return s;
}

double  getArea ( Component* component )
//**************************************
{
  Box  bb = component->getBoundingBox ();

  return getValue(bb.getWidth()) * getValue(bb.getHeight());
}




} // End of Hurricane namespace.

// ****************************************************************************************************
// Copyright (c) BULL S.A. 2000-2004, All Rights Reserved
// ****************************************************************************************************
