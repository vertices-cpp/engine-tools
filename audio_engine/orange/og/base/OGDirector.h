#ifndef _DIRECTOR_H_
#define _DIRECTOR_H_ 

#include "platform/OGPlatformMacros.h"
#include "base/OGScheduler.h"
#include "base/OGRef.h"
static int id = 0;

OG_BEGIN
  
class  Director :public Ref
{
public:
	
	Scheduler *_scheduler;
  
public:
	Instance(Director);

	Director();
	~Director();

	Scheduler* getScheduler() const { return _scheduler; }
	 

	void free();

	void init();
	  
	void mainLoop();
	 
};



OG_END

#endif
