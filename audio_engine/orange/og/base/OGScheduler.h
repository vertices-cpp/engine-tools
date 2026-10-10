
/**
 * @class Scheduler
 * @brief 主线程调度器。
 *
 * @note 线程模型：
 *       - 本类 **不是线程安全的**。
 *       - 所有方法（schedule / unschedule / pauseTarget / resumeTarget /
 *         setTimeScale / update ...）**只能在主线程调用**。
 *       - 其他线程若需要调度任务，必须通过
 *         `performFunctionInOrangeThread()` 投递到主线程执行。
 *       - 跨线程直接调用本类方法会导致数据竞争、崩溃或未定义行为。
 */
#ifndef __OGSCHEDULER_H__
#define __OGSCHEDULER_H__

#include <functional>
#include <mutex>
#include <set>
#include <vector>
#include <string>


#include "base/OGRef.h"
//#include "base/OGVector.h"

#include "platform/OGPlatformMacros.h"
#include "base/uthash.h"

OG_BEGIN

class Scheduler;

typedef std::function<void(float)> SchedulerFunc;
class Timer : public Ref
{
protected:
	Timer();
public:
	void setupTimerWithInterval(float seconds, unsigned int repeat, float delay);
	void setAborted() { _aborted = true; }
	bool isAborted() const { return _aborted; }
	bool isExhausted() const;

	virtual void trigger(float dt) = 0;
	virtual void cancel() = 0;

	/** triggers the timer */
	void update(float dt);

protected:
	Scheduler* _scheduler; // weak ref
	float _elapsed;
	bool _runForever;
	bool _useDelay;
	unsigned int _timesExecuted;
	unsigned int _repeat; //0 = once, 1 is 2 x executed
	float _delay;
	float _interval;
	bool _aborted;
};


class TimerTargetSelector : public Timer
{
public:
	TimerTargetSelector();

	/** Initializes a timer with a target, a selector and an interval in seconds, repeat in number of times to repeat, delay in seconds. */
	bool initWithSelector(Scheduler* scheduler, SEL_SCHEDULE selector, Ref* target, float seconds, unsigned int repeat, float delay);

	SEL_SCHEDULE getSelector() const { return _selector; }

	virtual void trigger(float dt) override;
	virtual void cancel() override;

protected:
	Ref* _target;
	SEL_SCHEDULE _selector;
};


class TimerTargetCallback : public Timer
{
public:
	TimerTargetCallback();

	// Initializes a timer with a target, a lambda and an interval in seconds, repeat in number of times to repeat, delay in seconds.
	bool initWithCallback(Scheduler* scheduler, const SchedulerFunc& callback, void *target, const std::string& key, float seconds, unsigned int repeat, float delay);

	const SchedulerFunc& getCallback() const { return _callback; }
	const std::string& getKey() const { return _key; }

	virtual void trigger(float dt) override;
	virtual void cancel() override;

protected:
	void* _target;
	SchedulerFunc _callback;
	std::string _key;
};

struct _listEntry;
struct _hashSelectorEntry;
struct _hashUpdateEntry;

class Scheduler : public Ref
{
public:

	static const int PRIORITY_SYSTEM;


	static const int PRIORITY_NON_SYSTEM_MIN;

	Scheduler();
	virtual ~Scheduler();

	/**
	 * Gets the time scale of schedule callbacks.
	 * @see Scheduler::setTimeScale()
	 */
	float getTimeScale() { return _timeScale; }

	void setTimeScale(float timeScale) { _timeScale = timeScale; }


	void update(float dt);


	void schedule(const SchedulerFunc& callback, void *target, float interval, unsigned int repeat, float delay, bool paused, const std::string& key);


	void schedule(const SchedulerFunc& callback, void *target, float interval, bool paused, const std::string& key);


	void schedule(SEL_SCHEDULE selector, Ref *target, float interval, unsigned int repeat, float delay, bool paused);


	void schedule(SEL_SCHEDULE selector, Ref *target, float interval, bool paused);


	template <class T>
	void scheduleUpdate(T *target, int priority, bool paused)
	{
		this->schedulePerFrame([target](float dt) {
			target->update(dt);
		}, target, priority, paused);
	}


	void unschedule(const std::string& key, void *target);


	void unschedule(SEL_SCHEDULE selector, Ref *target);


	void unscheduleUpdate(void *target);

	void unscheduleAllForTarget(void *target);

	void unscheduleAll();

	void unscheduleAllWithMinPriority(int minPriority);


	bool isScheduled(const std::string& key, const void *target) const;


	bool isScheduled(SEL_SCHEDULE selector, const Ref *target) const;


	void pauseTarget(void *target);

	void resumeTarget(void *target);

	bool isTargetPaused(void *target);

	std::set<void*> pauseAllTargets();
	std::set<void*> pauseAllTargetsWithMinPriority(int minPriority);
	void resumeTargets(const std::set<void*>& targetsToResume);
	void performFunctionInOrangeThread(std::function<void()> function);
	void removeAllFunctionsToBePerformedInOrangeThread();

protected:

	void schedulePerFrame(const SchedulerFunc& callback, void *target, int priority, bool paused);

	void removeHashElement(struct _hashSelectorEntry *element);
	void removeUpdateFromHash(struct _listEntry *entry);

	// update specific

	void priorityIn(struct _listEntry **list, const SchedulerFunc& callback, void *target, int priority, bool paused);
	void appendIn(struct _listEntry **list, const SchedulerFunc& callback, void *target, bool paused);


	float _timeScale;

	//
	// "updates with priority" stuff
	//
	struct _listEntry *_updatesNegList;        // list of priority < 0
	struct _listEntry *_updates0List;            // list priority == 0
	struct _listEntry *_updatesPosList;        // list priority > 0
	struct _hashUpdateEntry *_hashForUpdates; // hash used to fetch quickly the list entries for pause,delete,etc
	std::vector<struct _listEntry *> _updateDeleteVector; // the vector holds list entries that needs to be deleted after update

	// Used for "selectors with interval"
	struct _hashSelectorEntry *_hashForTimers;
	struct _hashSelectorEntry *_currentTarget;
	bool _currentTargetSalvaged;
	// If true unschedule will not remove anything from a hash. Elements will only be marked for deletion.
	bool _updateHashLocked;

	// Used for "perform Function"
	std::vector<std::function<void()>> _functionsToPerform;
	std::mutex _performMutex;
};

// end of base group
/** @} */

OG_END

#endif // __OGSCHEDULER_H__
