#ifndef gear2d_definitions_h
#define gear2d_definitions_h

#include <vector>
#include <set>
#include <list>
#include <exception>

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <ios>
#include <cctype>
#include <functional>
//#include <locale>

/** 
 * @file definitions.h
 * @brief Common definitions and useful functions.
 * @author Leonardo Guilherme de Freitas
 * 
 * This header contains useful functions like split, some forward
 * declarations and common definitions used thoughout the engine */

/* Adjust visibility modifiers depending on compiler */
#if defined(_WIN32)
# if !defined(__GNUC__)
#   pragma warning(1 : 4519) /* VC++, please... */
#   pragma warning(disable : 4514)
#   pragma warning(disable : 4820);
# endif
#   if defined(gear2d_EXPORTS) /* defined by cmake, thanks god. */
#       define  g2dapi  __declspec(dllexport) 
#   else
#       define  g2dapi  __declspec(dllimport) 
#   endif
#else
#   define g2dapi
#endif

namespace gear2d {
  class object;
  namespace component { 
    class base;
    
    /**
     * @brief Definition of a parameter handler.
     * 
     * This typedef specifies the signature for a component member function
     * pointer to be called when you hook to a parameter. Using this specification
     * you can create custom callbacks other than handle(). 
     * 
     * @warning Note that even complying to this signature, you still need
     * to cast your callback to component::base::call when hooking it.
     * 
     * @code
     * class test : public component::base {
     *   public:
     *      // example of callback
     *      virtual void xcallback(std::string pid, gear2d::component::base * lastwrite, gear2d::object * owner);
     *   public:
     *      setup() {
     *        // example on how to register it
     *        hook("x", (component::base::call)&xcallback);
     *      }
     * };
     * @endcode
     */
    typedef void (component::base::*call)(std::string pid, component::base * lastwrite, gear2d::object * owner);
  }
  
  /**
   * @var typedef float timediff
   * @brief Type used to identify time difference in seconds (SECONDS!). */
  typedef float timediff;
  
  /**
   * @var typedef std::string filename 
   * @brief Type that defines a file name. */
  typedef std::string filename;
  
  
  
  /**
   * @fn void clamp(T & value, const T min, const T max)
   * @brief Clamp a value between its max and min.
   * @param value Value to be clamped
   * @param min Minimum value. If <code>value</code> is less than it, it will be set to <code>min</code>
   * @param max Maximum value. If <code>value</code> is greater than it, it will be set to <code>max</code>
   * @return Clamped value
   * @relates gear2d::component::base
   */
  template <typename T>
  T & clamp(T & value, const T & min, const T & max) {
    if (value > max) value = max;
    if (value < min) value = min;
    return value;
  }
  
  /**
   * @fn T max(T & x, T & y)
   * @param x First value to compare
   * @param y Second value to compare 
   * @brief return the max value between x and y */
  template <typename T>
  T & max(T & x, T & y) {
    if (x < y) return y;
    else return x;
  }
  
  /**
   * @fn T max(T & x, T & y)
   * @param x First value to compare
   * @param y Second value to compare 
   * @brief return the minimum value between x and y */
  template <typename T>
  T & min(T & x, T & y) {
    if (x < y) return x;
    else return y;
  }  
  
  /**
   * @brief Evil class.
   * 
   * This is an evil class, so you can catch the evil whenever
   * you try good things. 
   * 
   * This exception is thrown whenever an irrecoverable error occurs.
   * Ideally the <i>modus operandi</i> for the engine is to fail
   * silently and continue running, but some errors throw this exception,
   * like when a required component is not found.
   *
   */
  class g2dapi evil : public std::exception {
    private:
      std::string describe;
      
    public:
      /**
       * @brief Evil constructor.
       * @param describe human-readable description of the evil that just happened.
       */
      evil(std::string describe = "Something evil happened!") throw() : describe(describe) { }
      virtual ~evil() throw() { };
      
      /**
       * @brief Describes the error.
       * @returns An c-type string with the error description
       */
      virtual const char* what() const throw() { return describe.c_str(); }
  };
  
  /**
   * @fn container_t & split(container_t & elems, const std::string & s, char delim)
   * @brief String splitter to any container.
   * @param elems Target element container.
   * @param s String to be splitted.
   * @param delim Delimiting char.
   * @return elems
   * @relates gear2d::component::base
   * 
   * Uses stringstream and getline to split a string using the given delimiter.
   */
  template <typename container_t>
  container_t & split(container_t & elems, const std::string & s, char delim) {
    std::stringstream ss(s);
    std::string item;
    std::insert_iterator<container_t> it(elems, elems.begin());
    while(std::getline(ss, item, delim)) {
      if (item.length() > 0) {
        *it = item;
        it++;
      }
    }
    return elems;
  }
  
  /**
   * @fn container_t split(const std::string & s, char delim)
   * @brief Handy shortcut for the split function.
   * @param s String to be splitted.
   * @param delim Delimiting char.
   * @return Instance of the container
   * @relates gear2d::component::base
   */
  template <typename container_t = std::list<std::string>>
  container_t split(const std::string & s, char delim) {
    container_t elems;
    return split(elems, s, delim);
  }
  
  /**
   * @brief Transform a string to its lower-case version.
   * @param target String that will be transformed
   * @warning This funcion is not locale-safe!
   */
  static inline std::string & strtolower(std::string & target) {
    std::transform(target.begin(), target.end(), target.begin(), ::tolower);
    return target;
  }
  
  
  /**
   * @fn datatype eval(std::string raw, datatype def)
   * @brief Evaluate a string to a type
   * @param raw Raw string to be evaluated
   * @param def Default if raw is empty
   * @relates gear2d::component::base
   * 
   * This tries to do a lexical cast from the raw string to the
   * destination (datatype) type.
   * 
   * You can (and should!) do a template specialization of this method
   * if you know better than this function. Example:
   * @code
   * template<> eval<your-fancy-customtype>(std::string rawstr, your-fancy-customtype def) {
   *     // do your stuff to convert raw string to your-fancy-type
   *     // and then return it
   * }
   * @endcode
   * 
   * The default works very well for basic types.
   * 
   * @warning The lexical cast may fail if converting
   * from string to some of the basic types like float
   * or int if raw is empty. Check your arguments.
   */
  template<typename datatype>
  datatype eval(const std::string & raw, const datatype & def = datatype()) {
    std::stringstream sstr;
    datatype t;
    sstr << raw;
    sstr >> t;
    if (sstr.fail()) t = def;
    return t;
  }
  
  template<>
  inline std::string eval<std::string>(const std::string & raw, const std::string & def) {
    if (raw.empty()) return def;
    else return raw;
  }
  
  template<>
  inline bool eval<bool>(const std::string & raw, const bool & def) {
    if (raw.empty()) return def;
    std::string e = raw; strtolower(e);
    if (e.compare("true") == 0 || raw[0] == '1') return true;
    else return false;
  }

 /**
  * @brief Left trim a std::string.
  * @param s the string to trim
  * @return s left trimmed */
  static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
  }

  /**
  * @brief Right trim a std::string
  * @param s the string to trim
  * @return s right trimmed */
  static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
  }

 /**
  * @brief Trim a std::string left and right
  * @param s the string to trim
  * @return s trimmed */
  static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
  }
}
#endif
