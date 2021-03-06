#ifndef gear2d_component_h
#define gear2d_component_h

#include <string>
#include <map>
#include <list>
#include <iostream>
using std::cout;
using std::endl;

#include <iostream>

#include "definitions.h"
#include "parameter.h"
#include "object.h"

/** 
 * @file component.h
 * @author Leonardo Guilherme de Freitas
 * @brief This file contains the base component interface.
 * 
 * Every component MUST implement this base class if its meant
 * to be a component.
 */


/** Macros to ease the process of declaring a new component */
#if defined (_WIN32) && defined (_MSC_VER)
#  define g2dcomponent(x, g2dfamily, g2dtype) extern "C" { __declspec(dllexport) gear2d::component::base * g2dfamily##_##g2dtype##_##build() { return new x; } }
#else
#  define g2dcomponent(x, g2dfamily, g2dtype) extern "C" { gear2d::component::base * g2dfamily##_##g2dtype##_##build() { return new x; } }
#endif


namespace gear2d {
  class object;
  class engine;

  /**
   * @namespace component
   * @brief Component namespace
   * 
   ** Where all components shall live.
   ** Every component should be declared within
   ** this namespace.
   **/
  namespace component {
    
    /** @brief Component type identifier */
    typedef std::string type;
    
    /** @brief Component identifyier type */
    typedef component::base * id;
    
    /** @brief Component family identifier */
    typedef std::string family;
    
    /**
     * @brief Helper class for handling type/id pairs
     * Use this class whenever you want to be very specific
     * about component description. This helps converting
     * from string to family/type selector and vice-versa. */
    struct g2dapi selector {
      /** @brief Family of this selector */
      component::family family;
      
      /** @brief Type of this selector */
      component::type type;
      
      /** @brief Builds an empty selector */
      selector();
      
      /** 
       * @brief Builds a selector from a family and type
       * @param family Family of this component
       * @param type Type of this component
       * @warning type will defaults to family if not specified */
      selector(component::family family, component::type type);
      
      /**
       * @brief Builds a selector from a family/type string
       * @param selstring Selector string as in family/type */
      selector(std::string selstring);
      
      /**
       * @brief Convert a string as in family/type to this selector
       * @param rhs String to be converted
       * @return this
       */
      selector & operator=(std::string rhs);
      
      /**
       * @brief Copy a selector
       * @param rhs Other selector
       * @return this
       */
      selector & operator=(const selector & rhs);
      
      /**
       * @brief Compare family and type to other selector
       * @param rhs Other selector
       * @return true if family and type are equals, false if at least one is not equal */
      bool operator==(const selector & rhs);
      
      /**
       * @brief Compare family and type to a selector string
       * @param rhs Selector string
       * @return true if the decomposed string matches family/type. false if not. */
      bool operator==(std::string rhs);
      
      /** 
       * @brief Convert this selector to a family/type string
       * @return family/type string */
      operator std::string();
      
      ~selector() { }
    };
    
    /** 
     * @brief Base class for components.
     * All classes meant to be component shall
     * derive on this class as it provide the
     * common interface for all of them
     *
     * @warning Only one type of a component could
     * be added. If you need to group multiple components
     * of the same type, create a custom component as a
     * container of them.
     * 
     * @warning A component only receives update(), message
     * or anything when attached to a game object.
     * 
     */
    class g2dapi base {
      friend class gear2d::object;
      friend class gear2d::engine;
      friend class gear2d::component::factory;
      
      public:
        /**
         * @brief Class that helps parsing signatures.
         * Mainly used in the setup() phase of a component,
         * this class uses the object signature, the parameter
         * name and its default value to initialize a gear2d::link.
         * 
         * @example
         * @code
         * sigparser p(sig, this);
         * link<int> x = p.init("x", 0);
         * @endcode
         */
        class sigparser {
          private:
            object::signature * sig;
            component::base * com;

          public:
            sigparser(object::signature & sig, component::base * com)
              : sig(&sig)
              , com(com)
            { }
            
            sigparser()
              : sig(nullptr)
              , com(nullptr)
            { }

            std::string & operator[](const std::string & k) {
              return (*sig)[k];
            }
            
            template <typename datatype>
            link<datatype> init(std::string pid, const datatype & def = datatype()) {
              if (sig == nullptr || com == nullptr) throw evil("sigparser not initialized!");
              
              auto it = (*sig).find(pid);
              if (it != (*sig).end()) {
                return (*com).fetch<datatype>(pid, eval<datatype>(it->second, def));
              } else {
                return (*com).fetch<datatype>(pid, def);
              }
            }

            link<std::string> init(std::string pid, std::string def = std::string("")) {
              if (sig == nullptr || com == nullptr) throw evil("sigparser not initialized!");
              auto it = (*sig).find(pid);
              if (it != (*sig).end()) def = it->second;
              return (*com).fetch<std::string>(pid, def);
            }
        };

      
      protected:
        base();
        
      public:
        virtual ~base();
        
        /** @brief Returns the type of this component */
        virtual component::type type() = 0;
        
        /** @brief Returns the family of this component.
         * 
         * Defaults to type() */
        virtual component::family family() { return type(); }
        
        /**
         * @brief Components that must exists before its attachment.
         * 
         * The object will call this function in the attachment process,
         * and check dependencies one by one. If any of the specified
         * components is not found attached, attaching will simply fail */
        virtual std::string depends() { return std::string(); }
        
        /**
         * @brief Method that will be called to handle value changes.
         * @param pid Parameter ID of the changed parameter
         * @param lastwrite The last component to write in this parameter (likely the one who triggered it)
         * @param owner The owner of the parameter (where it comes from)
         * 
         * You should implement this in your component if you ever
         * happen to listen to a parameter. */
        virtual void handle(parameterbase::id pid, component::base * lastwrite, object::id owner);
        
        /**
         * @brief Where you should put initialization routines.
         * @param sig Object original signature
         * 
         * This is set to give your component an opportunity
         * to initialize (since its not done in the construction).
         * Using sig you can parse all the initial parameters
         * passed to the object via signature (loaded from the object
         * file).
         *
         * This is garanteed to be called after the object was attached, so
         * read, eval, and write operations will be garanteed to work.
         */
        virtual void setup(object::signature & sig) = 0;
        
        /**
         * @brief Updates the state of this component
         * @param dt Delta time relative to last frame
         */
        virtual void update(timediff dt) { }
        
        /**
         * @brief Updates the state of this component
         * @param dt Delta time relative to last frame.
         * @param begin Tick this frame began. */
        virtual void update(timediff dt, int begin) {
          this->update(dt);
        }
        
        /**
        * @brief Signals when the object is marked for destruction.
        * 
        * All parameters are still valid in this phase.
        */
        virtual void destroyed() { }
       
        /**
         * @brief Returns a link to the parameter.
         * @param pid Parameter id
         * @param init Value to init the parameter if it is not initialized
         * 
         * @warning You should call this only inside
         * setup() or after. Never at construction time.
         */
        template<typename basetype>
        link<basetype> fetch(const parameterbase::id & pid, const basetype & init = basetype()) {
          if (!exists(pid)) write(pid, init);
          parameter<basetype> * p = access<basetype>(pid);
          return link<basetype>(p, this);
        }
        
        /**
         * @brief Reads a shared parameter and store on target
         * @param pid Parameter id to be read
         * @param target Target where the parameter value will be put
         * @warning The parameter will be created if it does not exists
         * 
         * Reads a parameter and store it on target. */
        template<typename datatype>
        void read(const parameterbase::id & pid, datatype & target) {
          parameter<datatype> * v = access<datatype>(pid);
          target = v->get();
        }
        
        /**
         * @brief Reads a shared parameter and return its value
         * @param pid Parameter id to be read
         * @warning The parameter will be created if it does not exists */
        template<typename datatype>
        datatype read(const parameterbase::id & pid) {
          parameter<datatype> * v = access<datatype>(pid);
          return v->get();
        }
        
        
      public:
        /**
         * @brief Writes in a shared parameter pertaining to parent object.
         * @param pid Parameter id to be write to
         * @param source Source from it shall be copied
         * @warning Be careful about type promotion. Be explicit if necessary.
         * @warning The parameter will be created if it does not exists */
        template<typename datatype>
        void write(const parameterbase::id & pid, const datatype & source) {
          parameter<datatype> * v = access<datatype>(pid);
          v->lastwrite = this;
          v->set(source);
        }
        
        template<typename datatype>
        void write(parameterbase::id pid, const link<datatype> & source) {
          parameter<datatype> * v = access<datatype>(pid);
          v->lastwrite = this;
          v->set((datatype)source);
        }
        
      public:
        /**
         * @brief Writes in a shared parameter in another object
         * @param oid Object id that holds the parameter
         * @param pid Parameter id to be write to
         * @param source Source from it shall be copied
         * @warning Be careful about type promotion, never rely on it. Be explicit if necessary.
         * @warning The parameter will @b *NOT* be created if it does not exists.
         * This behaviour is different from void
         * the previous \ref write(parameterbase::id pid, const datatype & source) "write"
         * function.
         */
        template<typename datatype>
        void write(object::id oid, const parameterbase::id & pid, const datatype & source) {
          parameter<datatype> * v = (parameter<datatype> *) oid->get(pid);
          if (v == 0) return;
          v->lastwrite = this;
          v->set(source);
        }
        
        /**
         * @brief Perform read, add, write
         * @param pid Parameter to be added
         * @param sum Value to be added
         * @warning Be careful about type promotion. Be explicit if necessary.
         */
        template<typename datatype>
        void add(parameterbase::id pid, const datatype & sum) {
          datatype t;
          read(pid, t);
          write(pid, t + sum);
        }
        
        /**
         * @brief Initialize a parameter from string
         * @param pid Parameter id
         * @param raw Raw string value
         * @param def Default if raw is empty
         * 
         * Evaluates the raw string to datatype and then set the parameter
         * to it, using eval(). eval may be specialized to mach your specific
         * needs. Look at @ref eval for more info on how to do that.
         */
        template<typename datatype>
        void init(const parameterbase::id & pid, std::string raw, datatype def) {
          if (owner->get(pid) != 0) throw evil(this->type() + "tried init(" + pid + ")" + " but its already set.");
          if (raw == "") write(pid, def);
          else write(pid, eval<datatype>(raw, def));
          parameterbase::value v = owner->get(pid);
          v->lastwrite = this;
        }
        
        /**
         * @brief Uses another parameter to set the pid
         * @param pid Parameter id
         * @param raw Parameter raw data to be associated with pid
         * @deprecated
         * 
         * Sets pid to point to an existing raw parameter. Likely used to provide
         * data shared among many instances of the same component and meant to
         * be changed by others.
         *
         * @warning This is dangerous as hell because we allow referencing data
         * thats inside the parameter (even if its const) everywhere. Hell, this method
         * is even encouraging this kind of usage.  Using this twice will
         * make all the old references break in the ugliest form possible.
         * Thats why it WILL fail (throw evil at you) if the object
         * already have a parameter with the same name.
         * 
         * @warning Note that \ref read(), \ref write(), \ref add and \ref raw creates the parameter
         * if it do not exists, so be assured to use bind() before.
         */
        template <typename datatype>
        void bind(const parameterbase::id & pid, datatype & raw) throw (evil) {
          if (owner->get(pid) != 0) throw evil(this->type() + " tried bind(" + pid + ")" + " but its already set.");
          parameter<datatype> * p = new parameter<datatype>(&raw);
          p->dodestroy = false;
          owner->set(pid, p);
        }
        

        /**
         * @brief Access a reference to the raw parameter.
         * 
         * Access a const reference to the raw parameter so
         * you do not need to perform read() too many times
         * (which perform a lot of function calls, including
         * comparing strings).
         * 
         * @warning Please note that this will create the parameter
         * if it does not exists.
         */
        template<typename datatype>
        const datatype & raw(const parameterbase::id & pid) {
          parameter<datatype> * v = access<datatype>(pid);
          return *(*v);
        }

        /**
         * @brief Hook this component to a parameter in parent.
         * @param pid Parameter id
         
         * Add this component as a listener to the parameter, to be
         * called whenever it changes its value. 
         * 
         * @warning Be aware that if the
         * parameter does not exists, it will not be created and it
         * will fail silently. Use <code>if (exists(pid)) hook(pid)</code> if you
         * want to know if/when it fails. If you know the component where that
         * parameter is coming from, its better to add it to the dependency
         * list, no?
         */
        void hook(const parameterbase::id & pid) {
          hook(pid, 0);
        }
        
        void unhook(const parameterbase::id & pid) {
          parameterbase::value v = owner->get(pid);
          if (v == 0) return;
          v->unhook(this);
        }
        
        /**
         * @brief Hook this component as a listener in a parameter.
         * @param pid Parameter id
         * @param handlerfp Pointer to the method that will be called
         * 
         * Add this component as a listener to the parameter. @p handlerfp
         * will be called whenever parameter @p pid changes its value.
         * See component::call documentantion on how to use the handlerfp.
         *
         * @warning Be aware that if the
         * parameter does not exists, it will not be created and it
         * will fail silently. Use <code>if (exists(pid)) hook(pid)</code> if you
         * want to know if/when it fails.
         */
        void hook(const parameterbase::id & pid, component::call handlerfp) {
          parameterbase::value v = owner->get(pid);
          if (v == 0) return;
          v->hook(this, handlerfp);
        }
        
        /**
         * @brief Hook this component to a parameter on another component
         * @param c Component to hook in
         * @param pid Parameter id
         * 
         * Add this component as a listener to the parameter in another
         * component. Owner will be passed in handle() as a way of
         * knowing if it is a parameter on your object or on another */
        void hook(component::base * c, const parameterbase::id & pid) {
          if (c == 0) return;
          parameterbase::value v = c->owner->get(pid);
          if (v == 0) return;
          v->hook(this);
        }
        
        
        /**
         * @brief Unhook this component in a parameter in another component
         * @param c Component to unhook in
         * @param pid Parameter id */
        void unhook(component::base * c, const parameterbase::id & pid) {
          if (c == 0) return;
          parameterbase::value v = c->owner->get(pid);
          if (v == 0) return;
          v->unhook(this);
        }
        
        /**
        * @brief Hook this component to a parameter on another component using a custom handle function.
        * @param c Component to hook in
        * @param pid Parameter id
        * @param handlerfp Pointer to the method that will be called
        * 
        * Add this component as a listener to the parameter in another
        * component. Owner will be passed in handle() as a way of
        * knowing if it is a parameter on your object or on another */
        void hook(component::base * c, const parameterbase::id & pid, component::call handlerfp) {
          if (c == 0) return;
          parameterbase::value v = c->owner->get(pid);
          if (v == 0) return;
          v->hook(this, handlerfp);
        }
        
        /**
         * @brief Query if the given parameter exists.
         * @param pid Parameter id
         * @return parameter address if it does exists, (null) 0 if not.
         * This queries if the parameter exists. */
        parameterbase * exists(const parameterbase::id & pid);

        /**
         * @brief Build another component based on selector.
         * @param s Component selector (family/type)
         * 
         * Uses the component factory to build the component.
         * 
         * @warning Please notice that this will only work
         * if the component was built with a factory and will
         * only work at setup() time, not at constructing time. */
        component::base * build(component::selector s);
        
        /**
         * @brief Spawns another object.
         * @param t Type of the object to load
         * @return ID of the spawned object instance
         * 
         * This uses object factory to spawn another object
         * loaded.
         */
        object::id spawn(object::type t);
        
        /**
         * @brief Locate a living object in the game engine.
         * @param t type of the object to locate
         * @return ID-handler of the located object. 0 means not-found.
         * @warning You may want to use this on update() because
         * your parent object can be loaded AFTER the object you're loading.
         */
        object::id locate(object::type t);
        
        /**
         * @brief Clone the parent game object.
         * @return A pointer to a compononent of the same type that called clone()
         * 
         * This will creates a new game object and
         * then set its parameter table with this one. */
        component::base * clone();
        
        /**
         * @brief Marks the owner for destruction.
         * 
         * Marks the owner object for destruction at the next frame.
         * Please notice that this component will be put in the
         * destruction list too. */
        void destroy();
        
        /**
         * @brief Loads a new scene configuration.
         * @param scene Scene name
         */
        void load(std::string scene);
        
      private:
        template<typename datatype>
        parameter<datatype> * access(const parameterbase::id & pid) {
          parameterbase::value v = owner->get(pid);
          if (v == 0) {
            v = new parameter<datatype>;
            owner->set(pid, v);
          }
          return (parameter<datatype>*)v;
        }
        
        
      public:
        object::id owner;
        
      private:
        // component factory that build us
        component::factory * cfactory;
    };
    
    class factory {
      public:
        /**
         * @brief Path for system-wide components.
         * 
         * If a component fails to be found at component/, it will
         * be searched in compath */
        std::string compath;
        

      public:
        /** @brief The build function or functor that create that component */
        typedef base* (*builder)();
        
        /** @brief Adds a component builder to the factory.
         * 
         ** Register a component in the factory of components so it
         ** can be accessed by its name in build() */
        void set(component::selector s, factory::builder b);
        
        /**
         * @brief Build a component with the type t and family f
         * @param f Family of the component registered with \ref set
         * @param t Type of the component registered with \ref set
         * @return A pointer to the component
         * 
         * Use this whenever you want to build a component. If the
         * component was not found, a null pointer will be returned */
        base * build(component::selector s);
        
        /**
         * @brief Load a componend from a shared plugin
         * @param s A selector in the form family/type
         * @param filepath An optional argument specifying the plugin file
         * @throw evil */
        void load(component::selector s, std::string filepath = "") throw (evil);
        
        ~factory();
        
      private:
        typedef std::map<component::family, std::map<component::type, factory::builder> > buildertable;
        buildertable builders;
        
        typedef void * handler;
        typedef std::map<component::type, handler> handlertable;
        handlertable handlers;
    };
  }
}

#endif
