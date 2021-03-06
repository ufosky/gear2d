#include <fstream>
#include "object.h"
#include "component.h"
#include "engine.h"
#include "log.h"
#include "sigfile.h"

#include <exception>
#include <algorithm>

namespace gear2d {
  object::object(object::signature & sig)
   : destroyed(false)
   , sig(sig) {
  }
  
  object::~object() {
    // delete all components
    for (componentcontainer::iterator i = components.begin(); i != components.end(); i++) {
      component::base * c = i->second;
      c->destroyed();
      c->owner = 0;
      engine::remove(c);
      // lets the engine delete c;
    }
    
    // delete all properties
    for (parameterbase::table::iterator i = parameters.begin(); i != parameters.end(); i++) {
      parameterbase * p = i->second;
      if (p != 0) {
        parameters[p->pid] = 0;
        if (p->dodestroy)  {
          delete p;
        }
      }
    }
  }
  
  object::id object::oid() { return this; }
  std::string object::name() { return sig["name"]; }
  
  void object::attach(component::base * newc) throw (evildepends) {
    modinfo("object");
    if (newc == 0) return;
    
    trace("Attaching", newc->family(), newc->type());
    
    /* sees that dependencies are met */
    std::string depends = newc->depends();
    bool passed = true;
    std::string d;
    if (depends != "") {
      std::list<std::string> dependencies;
      split(dependencies, depends, ' ');
      while (!dependencies.empty()) {
        component::selector dependency = dependencies.front();
        
        /* se if family is here */
        component::base * requiredcom = components[dependency.family];
        if (requiredcom == 0) { 
          passed = false;
          d = dependency;
          break;
        }
        
        /* check if dependency has a type. if not, passed. if yes, check */
        if (dependency.type != "") {
          if (requiredcom->type() != dependency.type) {
            passed = false;
            d = dependency;
            break;
          }
        }
        dependencies.pop_front();
      }
    }
    
    if (passed == false) {
      std::string s;
      s = s + "Component " + newc->family() + "/" + newc->type() + " have unmet dependencies: " + d;
      throw (evildepends(s));
    }
    
    /* ok... dependency check has passed! */
    component::base * oldc = deattach(newc->family());
    if (oldc != 0) {
      trace("Detaching old component", oldc->type());
    }
    
    newc->owner = this;
    components[newc->family()] = newc;
    
    trace("Calling setup on", newc->type());
    newc->setup(sig);
    engine::add(newc);
  }
  
  component::base * object::deattach(component::family family) {
    component::base * oldc = components[family];
    if (oldc == 0) return 0;
    components.erase(family);
    engine::remove(oldc);
    return oldc;
  }

  parameterbase::value object::get(const parameterbase::id & pid) {
    return parameters[pid];
  }
  
  void object::set(const parameterbase::id & pid, parameterbase::value v) {
    parameters[pid] = v;
    if (v != 0) {
      v->owner = this;
      v->pid = pid;
    }
  }
  
  component::base * object::component(gear2d::component::family f) {
    return components[f];
  }
  
  void object::copy(object::id other) {
    if (other == 0) return;
    for (parameterbase::table::iterator it = parameters.begin(); it != parameters.end(); it++) {
      parameterbase::id pid = it->first;
      parameterbase::value pval = it->second;
      
      parameterbase::value otherval = other->get(pid);
      pval->set(otherval);
    }
  }
  
  void object::destroy() {
    destroyed = true;
    engine::destroy(this);
    ofactory->loadedobjs[sig["name"]].remove(this);
  }
  
  /* -- factory methods */
  object::factory::factory(component::factory & cfactory) : cfactory(cfactory) {

  }
  
  void object::factory::load(object::type objtype, bool reload) {
    modinfo("object-factory");
    if (signatures.find(objtype) != signatures.end() && !reload) {
      trace(objtype, "Already loaded.");
      return;
    }
    
    /* open the file */
    string filename = objpath + objtype + ".yaml";
    trace("Loading", objtype, "from", filename);

    // TODO: figure out a better way to fail from sigfile::load
    object::signature & sig = signatures[objtype];
    bool sigloaded = sigfile::load(filename, sig);
    if (!sigloaded) {
      return;
    }
    
    sig["name"] = objtype;
    
    /* parse possible references */
    for (auto p : sig) {
      if (p.second[0] == '$') {
        string ref = p.second.substr(1, string::npos);

        // find reference first in local file
        auto it = sig.find(ref);
        if (it != sig.end()) {
          sig[p.first] = it->second;
        } else {
          // local reference not found, load from commonsig.
          sig[p.first] = commonsig[ref];
        }
        trace("Replacing", p.first, "with", ref, "=>", sig[p.first], ".");
      }
    }
    
    // add the global signature
    sig.insert(commonsig.begin(), commonsig.end());
  }
  
  object::id object::factory::locate(object::type objtype) {
    if (loadedobjs[objtype].size() == 0) return 0;
    else return loadedobjs[objtype].front();
  }
  
  void object::factory::set(object::type objtype, object::signature sig) {
    signatures[objtype] = sig;
    return;
  }

  void object::factory::innerbuild(object * o, std::string depends) {
    modinfo("object-factory");
    trace("Object:", o->name(), "Depends:", depends);
    std::set<std::string> comlist;
    split(comlist, depends, ' ');
    while(comlist.begin() != comlist.end()) {
      component::selector s = *(comlist.begin());
      
      /* maybe the object already has these loaded */
      component::base * samecom = o->components[s.family];
      if (samecom != 0) {
        if ((s.type == "")) {
          comlist.erase(comlist.begin());
          continue;
        }
        else if ((samecom->type() == s.type)) {
          comlist.erase(comlist.begin());
          continue;
        }
      }
      
      /* samecom not found, continue normal attach proccess */
      component::base * c = cfactory.build(s);
      if (c == 0) {
        cfactory.load(s);
        c = cfactory.build(s);
      }
      
      if (c == 0) continue;
      
      /* first try to attach the component.
       * if something went wrong, try to load
       * its dependencies. if still going wrong, give up
       * and let it fail */
      try {
        o->attach(c);
      } catch (evildepends & e) {
        /* build dependencies... */
        trace(e.what(), log::info);
        innerbuild(o, c->depends());
        /* if that build did'nt fixed, fuck it. */
        o->attach(c);
      }
      
      comlist.erase(comlist.begin());
     }
     trace("Finished", o->name(), depends);
  }
  
  object::id object::factory::build(gear2d::object::type objtype) {
    moderr("object-factory");
    if (signatures.find(objtype) == signatures.end()) {
      trace("Could not load object", objtype);
      return 0;
    }
    
    
    /* now get the signature of this type */
    object::signature & signature = signatures[objtype];
    
    /* instantiate the object */
    object * obj = new object(signature);
    obj->ofactory = this;
    
    
    /* now get the attach string */
    std::string attachstring = signature["attach"];
    
    /* if nothing to attach, return the object */
    if (attachstring == "") return obj;
    
    
    /* recursive build method that takes care of dependency loading */
    innerbuild(obj, attachstring);
    loadedobjs[objtype].push_back(obj);
    return obj;
  }
  
  evildepends::evildepends (std::string describe) : evil (describe) { }

}
