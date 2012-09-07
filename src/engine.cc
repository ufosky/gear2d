#include "engine.h"
#include "component.h"
#include "object.h" 
#include "log.h"

#include "yaml-cpp/yaml.h"
#include <fstream>
#include <vector>
#include <string>
#include <cstdio>
#include <ctime>
#include "SDL.h"

#include <algorithm>

#ifndef GEAR2D_VERSION
#define GEAR2D_VERSION "undefined"
#warning GEAR2D_VERSION undefined! Gear2D should be compiled with -DGEAR2D_VERSION=<version>
#endif

extern "C" {
  const char * libraryversion = GEAR2D_VERSION;
}

namespace gear2d {
  std::map<std::string, std::string> * engine::config;
  component::factory * engine::cfactory;
  object::factory * engine::ofactory;
  std::map<component::type, std::set<component::base *> > * engine::components;
  std::set<component::base *> * engine::removedcom;
  std::set<object::id> * engine::destroyedobj;
  bool engine::initialized;
  bool engine::started;
  std::string * engine::nextscene;
  
  const char * engine::version() { return libraryversion; }
  
  int engine::eventfilter(const void * _ev) {
    const SDL_Event * ev = (const SDL_Event*)_ev;
    switch(ev->type) {
      case SDL_QUIT:
        started = !(ev->type == SDL_QUIT);
        return 1;
        break;
      default:
        break;
        
    }
    return 1;
  }

  void engine::add(component::base * c) {
    if (components == 0) {
      modwarn("engine");
      trace("Initialize the engine before attaching a component to an object");
    }
    if (c == 0) return;
    (*components)[c->family()].insert(c); 
  }

  void engine::remove(component::base * c, bool rightnow) {
    if (rightnow == false) removedcom->insert(c);
    else {
      (*components)[c->family()].erase(c);
      delete c;
    }
  }
  
  void engine::destroy(object::id oid) {
    destroyedobj->insert(oid);
  }
  
  void engine::init(bool force) {
    modwarn("engine");
    if (initialized == true && force == false) return;
    if (config != 0) delete config;
    srand(std::time(0));
    SDL_InitSubSystem(SDL_INIT_EVENTTHREAD | SDL_INIT_VIDEO);
    SDL_SetEventFilter((SDL_EventFilter)eventfilter);
    config = new std::map<std::string, std::string>;
    
//     log::globalverb = log::warning;
    
    // erase all the components
    if (components != 0) {
      for (map<component::family, std::set<component::base *> >::iterator i = components->begin();
         i != components->end();
         i++)
      {
        std::set<component::base *> & com = i->second;
        while (!com.empty()) {
          component::base * c = *(com.begin());
          if (c != 0) delete c;
          com.erase(c);
        }
      }
    }
    delete components;
    components = new std::map<component::family, std::set<component::base *> >;
    
    if (removedcom != 0) delete removedcom;
    removedcom = new std::set<component::base *>;
    
    if (destroyedobj != 0) delete destroyedobj;
    destroyedobj = new std::set<object::id>;

    if (ofactory != 0) delete ofactory;
    if (cfactory != 0) delete cfactory;
    cfactory = new component::factory;
    ofactory = new object::factory(*cfactory);
    initialized = true;
    started = false;
  }
  
  void engine::next(std::string configfile) {
    *nextscene = configfile;
  }
  
  void engine::load(std::string configfile) {
    modinfo("engine");
    std::ifstream fin(configfile.c_str());
    
    /* initialize yaml parser */
    YAML::Parser parser(fin);
    YAML::Node node;
    map<std::string, std::string> cfg;
    
    /* TODO: add other yaml features */
    while (parser.GetNextDocument(node)) node >> cfg;
    load(cfg);
    fin.close();
  }

  void engine::load(std::map<std::string, std::string> & cfg) {
    modinfo("engine");
    init(true);
    *config = cfg;
    
    /* get component-search path */
    std::string compath = (*config)["compath"];
    if (compath == "") {
      trace("Component path (compath:) not defined at the scene file.", log::error);
    }
    
    cfactory->compath = compath;
    config->erase("compath");
    
    /* pre-load some of the components */
    /* TODO: travel the compath looking for family/component */
    std::vector<std::string> comlist;
    if ((*config)["compreload"].size() != 0) {
      split(comlist, (*config)["compreload"], ' ');
      for (int i = 0; i < comlist.size(); i++) {
        trace("Pre-loading", comlist[i], log::info);
        cfactory->load(comlist[i]);
      }
      config->erase("compreload");
    }
    /* load the indicated objects */
    std::vector<object::type> objectlist;
    split(objectlist, (*config)["objects"], ' ');
    config->erase("objects");
    
    /* The rest is added to the object factory global parameters */
    ofactory->commonsig = *config;
    
    /* and now build the pointed objects */
    for (int i = 0; i < objectlist.size(); i++) {
      ofactory->load(objectlist[i]);
      ofactory->build(objectlist[i]);
    }
    if (nextscene) delete nextscene;
    nextscene = new std::string("");
  }
  
  int engine::run() {
    // make sure we init
    init();
    modinfo("engine");
    int begin = 0, end = 0, dt = 0;
    SDL_Event ev;
    started = true;
    while (started) {
      dt = end - begin;
      begin = SDL_GetTicks();
      timediff delta = dt/1000.0f;
      
      if (dt < 60/1000.0f) SDL_Delay(60/1000.0f - dt);
      SDL_PumpEvents();
      
      for (std::set<object::id>::iterator i = destroyedobj->begin(); i != destroyedobj->end(); i++) {
        /* this will push object's components to removedcom, hopefully. */
        delete (*i);
      }
      
      destroyedobj->clear();
      
      /* first remove components from the running pipeline */
      for (std::set<component::base *>::iterator i = removedcom->begin(); i != removedcom->end(); i++) {
        component::type f = (*i)->family();
        (*components)[f].erase((*i));

        if (*i != 0) delete (*i);
        
        // do not manage empty lists
        if ((*components)[f].size() == 0)
          components->erase(f);
      }
      
      // clear the removed list
      removedcom->clear();
      
      /* now update pipeline accordingly */
      std::map<component::family, std::set<component::base *> >::iterator comtpit;
      for (comtpit = components->begin(); comtpit != components->end(); comtpit++) {
        component::family f = comtpit->first;
        std::set<component::base *> & list = comtpit->second;
        int b = SDL_GetTicks();
        for (std::set<component::base*>::iterator comit = list.begin(); comit != list.end(); comit++) {
          (*comit)->update(delta);
        }
      }

      /* no components, quit the engine */
      if (components->empty()) started = false;
      
      /* shall load next scene */
      if (*nextscene != "") {
        load(nextscene->c_str());
        started = true;
      }
      
      SDL_Delay(2);
      end = SDL_GetTicks();
    }
    
    delete ofactory;
    delete cfactory;
	return 0;
  }
}