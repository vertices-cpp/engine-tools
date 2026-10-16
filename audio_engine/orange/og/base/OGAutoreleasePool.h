
#ifndef __AUTORELEASEPOOL_H__
#define __AUTORELEASEPOOL_H__

#include <vector>
#include <string>
#include "base/OGRef.h"

/**
 * @addtogroup base
 * @{
 */
OG_BEGIN


/**
 * A pool for managing autorelease objects.
 * @js NA
 */
class  AutoreleasePool
{
public:
  
    AutoreleasePool();
    
   
    AutoreleasePool(const std::string &name);
    
    ~AutoreleasePool();

     
    void addObject(Ref *object);

    
    void clear();
    
 
    bool isClearing() const { return _isClearing; };
 
    bool contains(Ref* object) const;
 
    void dump();
    
private:
    
    std::vector<Ref*> _managedObjectArray;
    std::string _name;
 
    bool _isClearing;
 
};

// end of base group
/** @} */

/**
 * @cond
 */
class  PoolManager
{
public:

    static PoolManager* getInstance();
    static void destroyInstance();
   
    AutoreleasePool *getCurrentPool() const;

    bool isObjectInPools(Ref* obj) const;

    friend class AutoreleasePool;
    
private:
    PoolManager();
    ~PoolManager();
    
    void push(AutoreleasePool *pool);
    void pop();
    
    static PoolManager* s_singleInstance;
    
    std::vector<AutoreleasePool*> _releasePoolStack;
};
/**
 * @endcond
 */

OG_END

#endif //__AUTORELEASEPOOL_H__
