#include "base/OGRef.h"
#include "base/OGAutoreleasePool.h"
//#include "base/ogMacros.h"

// #include "ogMacros.h"
// //#include "CCScriptSupport.h"
// 
// #if OG_REF_LEAK_DETECTION
// #include <algorithm>    // std::find
// #include <thread>
// #include <mutex>
// #include <vector>
// #endif
// 

OG_BEGIN

// #if OG_REF_LEAK_DETECTION
// static void trackRef(Ref* ref);
// static void untrackRef(Ref* ref);
// #endif

Ref::Ref()
	: _referenceCount(1) // when the Ref is created, the reference count of it is 1
{}

Ref::~Ref()
{

}

void Ref::retain()
{
	//OGASSERT(_referenceCount > 0, "reference count should be greater than 0");
	++_referenceCount;
}

void Ref::release()
{
	//OGASSERT(_referenceCount > 0, "reference count should be greater than 0");
	--_referenceCount;

	if (_referenceCount == 0)
	{

		auto poolManager = PoolManager::getInstance();
		if (!poolManager->getCurrentPool()->isClearing() && poolManager->isObjectInPools(this))
		{

		}

		delete this;
	}
}

Ref* Ref::autorelease()
{
	PoolManager::getInstance()->getCurrentPool()->addObject(this);
	return this;
}

unsigned int Ref::getReferenceCount() const
{
	return _referenceCount;
}


OG_END





