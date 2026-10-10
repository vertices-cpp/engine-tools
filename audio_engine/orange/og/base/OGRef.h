
#ifndef __BASE_OGREF_H__
#define __BASE_OGREF_H__

#include "platform/OGPlatformMacros.h"
//#include "ogConfig.h"

#define OG_REF_LEAK_DETECTION 0

/**
 * @addtogroup base
 * @{
 */

OG_BEGIN


class Ref;

/**
  * Interface that defines how to clone an Ref.
  * @lua NA
  * @js NA
  */
class  Clonable
{
public:
	/** Returns a copy of the Ref. */
	virtual Clonable* clone() const = 0;

	virtual ~Clonable() {};
};


class   Ref
{
public:

	void retain();


	void release();

	Ref* autorelease();

	unsigned int getReferenceCount() const;

protected:

	Ref();

public:

	virtual ~Ref();

protected:
	/// count of references
	unsigned int _referenceCount;

	friend class AutoreleasePool;

};

class Node;

typedef void (Ref::*OG_CallFunc)();
typedef void (Ref::*OG_CallFuncN)(Node*);
typedef void (Ref::*OG_CallFuncND)(Node*, void*);
typedef void (Ref::*OG_CallFuncO)(Ref*);
typedef void (Ref::*OG_MenuHandler)(Ref*);
typedef void (Ref::*SEL_SCHEDULE)(float);

#define OG_CALLFUNC_SELECTOR(_SELECTOR) static_cast<orange::OG_CallFunc>(&_SELECTOR)
#define OG_CALLFUNCN_SELECTOR(_SELECTOR) static_cast<orange::OG_CallFuncN>(&_SELECTOR)
#define OG_CALLFUNCND_SELECTOR(_SELECTOR) static_cast<orange::OG_CallFuncND>(&_SELECTOR)
#define OG_CALLFUNCO_SELECTOR(_SELECTOR) static_cast<orange::OG_CallFuncO>(&_SELECTOR)
#define OG_MENU_SELECTOR(_SELECTOR) static_cast<orange::OG_MenuHandler>(&_SELECTOR)
#define OG_SCHEDULE_SELECTOR(_SELECTOR) static_cast<orange::SEL_SCHEDULE>(&_SELECTOR)

OG_END
// end of base group
/** @} */

#endif // __BASE_OGREF_H__
