

#ifndef OG_ARRAY_H
#define OG_ARRAY_H
/// @cond DO_NOT_SHOW

//#include "base/ogMacros.h"
#include "base/OGRef.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>

OG_BEGIN

// Easy integration
#define OGARRAYDATA_FOREACH(__array__, __object__)															\
__object__=__array__->arr[0]; for(size_t i=0, num=__array__->num; i<num; i++, __object__=__array__->arr[i])	\


typedef struct _ogArray {
	size_t num, max;
	Ref** arr;
} ogArray;

/** Allocates and initializes a new array with specified capacity */
ogArray* ogArrayNew(size_t capacity);

/** Frees array after removing all remaining objects. Silently ignores nil arr. */
void ogArrayFree(ogArray*& arr);

/** Doubles array capacity */
void ogArrayDoubleCapacity(ogArray *arr);

/** Increases array capacity such that max >= num + extra. */
void ogArrayEnsureExtraCapacity(ogArray *arr, size_t extra);

/** shrinks the array so the memory footprint corresponds with the number of items */
void ogArrayShrink(ogArray *arr);

/** Returns index of first oogurrence of object, NSNotFound if object not found. */
size_t ogArrayGetIndexOfObject(ogArray *arr, Ref* object);

/** Returns a Boolean value that indicates whether object is present in array. */
bool ogArrayContainsObject(ogArray *arr, Ref* object);

/** Appends an object. Behavior undefined if array doesn't have enough capacity. */
void ogArrayAppendObject(ogArray *arr, Ref* object);

/** Appends an object. Capacity of arr is increased if needed. */
void ogArrayAppendObjectWithResize(ogArray *arr, Ref* object);

/** Appends objects from plusArr to arr. 
 Behavior undefined if arr doesn't have enough capacity. */
void ogArrayAppendArray(ogArray *arr, ogArray *plusArr);

/** Appends objects from plusArr to arr. Capacity of arr is increased if needed. */
void ogArrayAppendArrayWithResize(ogArray *arr, ogArray *plusArr);

/** Inserts an object at index */
void ogArrayInsertObjectAtIndex(ogArray *arr, Ref* object, size_t index);

/** Swaps two objects */
void ogArraySwapObjectsAtIndexes(ogArray *arr, size_t index1, size_t index2);

/** Removes all objects from arr */
void ogArrayRemoveAllObjects(ogArray *arr);

/** Removes object at specified index and pushes back all subsequent objects.
 Behavior undefined if index outside [0, num-1]. */
void ogArrayRemoveObjectAtIndex(ogArray *arr, size_t index, bool releaseObj = true);

/** Removes object at specified index and fills the gap with the last object,
 thereby avoiding the need to push back subsequent objects.
 Behavior undefined if index outside [0, num-1]. */
void ogArrayFastRemoveObjectAtIndex(ogArray *arr, size_t index);

void ogArrayFastRemoveObject(ogArray *arr, Ref* object);

/** Searches for the first oogurrence of object and removes it. If object is not
 found the function has no effect. */
void ogArrayRemoveObject(ogArray *arr, Ref* object, bool releaseObj = true);

/** Removes from arr all objects in minusArr. For each object in minusArr, the
 first matching instance in arr will be removed. */
void ogArrayRemoveArray(ogArray *arr, ogArray *minusArr);

/** Removes from arr all objects in minusArr. For each object in minusArr, all
 matching instances in arr will be removed. */
void ogArrayFullRemoveArray(ogArray *arr, ogArray *minusArr);

// 
// // ogCArray for Values (c structures)

typedef struct _ogCArray {
    size_t num, max;
    void** arr;
} ogCArray;

/** Allocates and initializes a new C array with specified capacity */
ogCArray* ogCArrayNew(size_t capacity);

/** Frees C array after removing all remaining values. Silently ignores nil arr. */
void ogCArrayFree(ogCArray *arr);

/** Doubles C array capacity */
void ogCArrayDoubleCapacity(ogCArray *arr);

/** Increases array capacity such that max >= num + extra. */
void ogCArrayEnsureExtraCapacity(ogCArray *arr, size_t extra);

/** Returns index of first oogurrence of value, NSNotFound if value not found. */
size_t ogCArrayGetIndexOfValue(ogCArray *arr, void* value);

/** Returns a Boolean value that indicates whether value is present in the C array. */
bool ogCArrayContainsValue(ogCArray *arr, void* value);

/** Inserts a value at a certain position. Behavior undefined if array doesn't have enough capacity */
void ogCArrayInsertValueAtIndex( ogCArray *arr, void* value, size_t index);

/** Appends an value. Behavior undefined if array doesn't have enough capacity. */
void ogCArrayAppendValue(ogCArray *arr, void* value);

/** Appends an value. Capacity of arr is increased if needed. */
void ogCArrayAppendValueWithResize(ogCArray *arr, void* value);

/** Appends values from plusArr to arr. Behavior undefined if arr doesn't have
 enough capacity. */
void ogCArrayAppendArray(ogCArray *arr, ogCArray *plusArr);

/** Appends values from plusArr to arr. Capacity of arr is increased if needed. */
void ogCArrayAppendArrayWithResize(ogCArray *arr, ogCArray *plusArr);

/** Removes all values from arr */
void ogCArrayRemoveAllValues(ogCArray *arr);

/** Removes value at specified index and pushes back all subsequent values.
 Behavior undefined if index outside [0, num-1].
 @since v0.99.4
 */
void ogCArrayRemoveValueAtIndex(ogCArray *arr, size_t index);

/** Removes value at specified index and fills the gap with the last value,
 thereby avoiding the need to push back subsequent values.
 Behavior undefined if index outside [0, num-1].
 @since v0.99.4
 */
void ogCArrayFastRemoveValueAtIndex(ogCArray *arr, size_t index);

/** Searches for the first oogurrence of value and removes it. If value is not found the function has no effect.
 @since v0.99.4
 */
void ogCArrayRemoveValue(ogCArray *arr, void* value);

/** Removes from arr all values in minusArr. For each Value in minusArr, the first matching instance in arr will be removed.
 @since v0.99.4
 */
void ogCArrayRemoveArray(ogCArray *arr, ogCArray *minusArr);

/** Removes from arr all values in minusArr. For each value in minusArr, all matching instances in arr will be removed.
 @since v0.99.4
 */
void ogCArrayFullRemoveArray(ogCArray *arr, ogCArray *minusArr);

OG_END
	
/// @endcond
#endif // CC_ARRAY_H
