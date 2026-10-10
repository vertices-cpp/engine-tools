

#include "base/ogCArray.h"
//#include "base/ogTypes.h"

OG_BEGIN

/** Allocates and initializes a new array with specified capacity */
ogArray* ogArrayNew(size_t capacity)
{
	if (capacity == 0)
		capacity = 7;
	
	ogArray *arr = (ogArray*)malloc( sizeof(ogArray) );
	arr->num = 0;
	arr->arr =  (Ref**)calloc(capacity, sizeof(Ref*));
	arr->max = capacity;
	
	return arr;
}

/** Frees array after removing all remaining objects. Silently ignores nullptr arr. */
void ogArrayFree(ogArray*& arr)
{
    if( arr == nullptr ) 
    {
        return;
    }
	ogArrayRemoveAllObjects(arr);
	
	free(arr->arr);
	free(arr);

    arr = nullptr;
}

void ogArrayDoubleCapacity(ogArray *arr)
{
	arr->max *= 2;
	Ref** newArr = (Ref**)realloc( arr->arr, arr->max * sizeof(Ref*) );
	// will fail when there's not enough memory
    //CCASSERT(newArr != 0, "ogArrayDoubleCapacity failed. Not enough memory");
	arr->arr = newArr;
}

void ogArrayEnsureExtraCapacity(ogArray *arr, size_t extra)
{
	while (arr->max < arr->num + extra)
    {
      //  CCLOGINFO("orange: ogCArray: resizing ogArray capacity from [%zd] to [%zd].",
//               arr->max,
//               arr->max*2);

		ogArrayDoubleCapacity(arr);
    }
}

void ogArrayShrink(ogArray *arr)
{
    size_t newSize = 0;
	
	//only resize when necessary
	if (arr->max > arr->num && !(arr->num==0 && arr->max==1))
	{
		if (arr->num!=0)
		{
			newSize=arr->num;
			arr->max=arr->num;
		}
		else
		{//minimum capacity of 1, with 0 elements the array would be free'd by realloc
			newSize=1;
			arr->max=1;
		}
		
		arr->arr = (Ref**)realloc(arr->arr,newSize * sizeof(Ref*) );
		//CCASSERT(arr->arr!=nullptr,"could not reallocate the memory");
	}
}
 
/** Returns index of first oogurrence of object, -1 if object not found. */
size_t ogArrayGetIndexOfObject(ogArray *arr, Ref* object)
{
    const auto arrNum = arr->num;
    Ref** ptr = arr->arr;
	for (size_t i = 0; i < arrNum; ++i, ++ptr)
    {
		if (*ptr == object)
            return i;
    }
    
	return -1;
}

/** Returns a Boolean value that indicates whether object is present in array. */
bool ogArrayContainsObject(ogArray *arr, Ref* object)
{
	return ogArrayGetIndexOfObject(arr, object) != -1;
}

/** Appends an object. Behavior undefined if array doesn't have enough capacity. */
void ogArrayAppendObject(ogArray *arr, Ref* object)
{
    //CCASSERT(object != nullptr, "Invalid parameter!");
    object->retain();
	arr->arr[arr->num] = object;
	arr->num++;
}

/** Appends an object. Capacity of arr is increased if needed. */
void ogArrayAppendObjectWithResize(ogArray *arr, Ref* object)
{
	ogArrayEnsureExtraCapacity(arr, 1);
	ogArrayAppendObject(arr, object);
}

/** Appends objects from plusArr to arr. Behavior undefined if arr doesn't have
 enough capacity. */
void ogArrayAppendArray(ogArray *arr, ogArray *plusArr)
{
	for (size_t i = 0; i < plusArr->num; i++)
    {
		ogArrayAppendObject(arr, plusArr->arr[i]);
    }
}

/** Appends objects from plusArr to arr. Capacity of arr is increased if needed. */
void ogArrayAppendArrayWithResize(ogArray *arr, ogArray *plusArr)
{
	ogArrayEnsureExtraCapacity(arr, plusArr->num);
	ogArrayAppendArray(arr, plusArr);
}

/** Inserts an object at index */
void ogArrayInsertObjectAtIndex(ogArray *arr, Ref* object, size_t index)
{
	//CCASSERT(index<=arr->num, "Invalid index. Out of bounds");
	//CCASSERT(object != nullptr, "Invalid parameter!");

	ogArrayEnsureExtraCapacity(arr, 1);
	
	size_t remaining = arr->num - index;
	if (remaining > 0)
    {
		memmove((void *)&arr->arr[index+1], (void *)&arr->arr[index], sizeof(Ref*) * remaining );
    }

    object->retain();
	arr->arr[index] = object;
	arr->num++;
}

/** Swaps two objects */
void ogArraySwapObjectsAtIndexes(ogArray *arr, size_t index1, size_t index2)
{
	//CCASSERT(index1>=0 && index1 < arr->num, "(1) Invalid index. Out of bounds");
	//CCASSERT(index2>=0 && index2 < arr->num, "(2) Invalid index. Out of bounds");
	
	Ref* object1 = arr->arr[index1];
	
	arr->arr[index1] = arr->arr[index2];
	arr->arr[index2] = object1;
}

/** Removes all objects from arr */
void ogArrayRemoveAllObjects(ogArray *arr)
{
	while (arr->num > 0)
    {
		(arr->arr[--arr->num])->release();
    }
}

/** Removes object at specified index and pushes back all subsequent objects.
 Behavior undefined if index outside [0, num-1]. */
void ogArrayRemoveObjectAtIndex(ogArray *arr, size_t index, bool releaseObj/* = true*/)
{
    //CCASSERT(arr && arr->num > 0 && index>=0 && index < arr->num, "Invalid index. Out of bounds");
    if (releaseObj)
    {
        OG_SAFE_RELEASE(arr->arr[index]);
    }
    
	arr->num--;
	
	size_t remaining = arr->num - index;
	if(remaining>0)
    {
		memmove((void *)&arr->arr[index], (void *)&arr->arr[index+1], remaining * sizeof(Ref*));
    }
}

/** Removes object at specified index and fills the gap with the last object,
 thereby avoiding the need to push back subsequent objects.
 Behavior undefined if index outside [0, num-1]. */
void ogArrayFastRemoveObjectAtIndex(ogArray *arr, size_t index)
{
	OG_SAFE_RELEASE(arr->arr[index]);
	auto last = --arr->num;
	arr->arr[index] = arr->arr[last];
}

void ogArrayFastRemoveObject(ogArray *arr, Ref* object)
{
	auto index = ogArrayGetIndexOfObject(arr, object);
	if (index != -1)
    {
		ogArrayFastRemoveObjectAtIndex(arr, index);
    }
}

/** Searches for the first oogurrence of object and removes it. If object is not
 found the function has no effect. */
void ogArrayRemoveObject(ogArray *arr, Ref* object, bool releaseObj/* = true*/)
{
	auto index = ogArrayGetIndexOfObject(arr, object);
	if (index != -1)
    {
		ogArrayRemoveObjectAtIndex(arr, index, releaseObj);
    }
}

/** Removes from arr all objects in minusArr. For each object in minusArr, the
 first matching instance in arr will be removed. */
void ogArrayRemoveArray(ogArray *arr, ogArray *minusArr)
{
	for (size_t i = 0; i < minusArr->num; i++)
    {
		ogArrayRemoveObject(arr, minusArr->arr[i]);
    }
}

/** Removes from arr all objects in minusArr. For each object in minusArr, all
 matching instances in arr will be removed. */
void ogArrayFullRemoveArray(ogArray *arr, ogArray *minusArr)
{
	size_t back = 0;
	
	for (size_t i = 0; i < arr->num; i++)
    {
		if (ogArrayContainsObject(minusArr, arr->arr[i]))
        {
			OG_SAFE_RELEASE(arr->arr[i]);
			back++;
		} 
        else
        {
			arr->arr[i - back] = arr->arr[i];
        }
	}
	
	arr->num -= back;
}

// 
// // ogCArray for Values (c structures)

/** Allocates and initializes a new C array with specified capacity */
ogCArray* ogCArrayNew(size_t capacity)
{
	if (capacity == 0)
    {
		capacity = 7;
    }

	ogCArray *arr = (ogCArray*)malloc(sizeof(ogCArray));
	arr->num = 0;
	arr->arr = (void**)malloc(capacity * sizeof(void*));
	arr->max = capacity;
	
	return arr;
}

/** Frees C array after removing all remaining values. Silently ignores nullptr arr. */
void ogCArrayFree(ogCArray *arr)
{
    if (arr == nullptr)
    {
        return;
    }
	ogCArrayRemoveAllValues(arr);
	
	free(arr->arr);
	free(arr);
}

/** Doubles C array capacity */
void ogCArrayDoubleCapacity(ogCArray *arr)
{
    ogArrayDoubleCapacity((ogArray*)arr);
}

/** Increases array capacity such that max >= num + extra. */
void ogCArrayEnsureExtraCapacity(ogCArray *arr, size_t extra)
{
    ogArrayEnsureExtraCapacity((ogArray*)arr,extra);
}

/** Returns index of first oogurrence of value, -1 if value not found. */
size_t ogCArrayGetIndexOfValue(ogCArray *arr, void* value)
{
	for(size_t i = 0; i < arr->num; i++)
    {
		if( arr->arr[i] == value )
            return i;
    }
	return -1;
}

/** Returns a Boolean value that indicates whether value is present in the C array. */
bool ogCArrayContainsValue(ogCArray *arr, void* value)
{
	return ogCArrayGetIndexOfValue(arr, value) != -1;
}

/** Inserts a value at a certain position. Behavior undefined if array doesn't have enough capacity */
void ogCArrayInsertValueAtIndex( ogCArray *arr, void* value, size_t index)
{
	//CCASSERT( index < arr->max, "ogCArrayInsertValueAtIndex: invalid index");
	
	auto remaining = arr->num - index;
    // make sure it has enough capacity
    if (arr->num + 1 == arr->max)
    {
        ogCArrayDoubleCapacity(arr);
    }
	// last Value doesn't need to be moved
	if( remaining > 0) {
		// tex coordinates
		memmove((void *)&arr->arr[index+1], (void *)&arr->arr[index], sizeof(void*) * remaining );
	}
	
	arr->num++;
	arr->arr[index] = value;
}

/** Appends an value. Behavior undefined if array doesn't have enough capacity. */
void ogCArrayAppendValue(ogCArray *arr, void* value)
{
	arr->arr[arr->num] = value;
	arr->num++;
    // double the capacity for the next append action
    // if the num >= max
    if (arr->num >= arr->max)
    {
        ogCArrayDoubleCapacity(arr);
    }
}

/** Appends an value. Capacity of arr is increased if needed. */
void ogCArrayAppendValueWithResize(ogCArray *arr, void* value)
{
	ogCArrayEnsureExtraCapacity(arr, 1);
	ogCArrayAppendValue(arr, value);
}


/** Appends values from plusArr to arr. Behavior undefined if arr doesn't have
 enough capacity. */
void ogCArrayAppendArray(ogCArray *arr, ogCArray *plusArr)
{
	for( size_t i = 0; i < plusArr->num; i++)
    {
		ogCArrayAppendValue(arr, plusArr->arr[i]);
    }
}

/** Appends values from plusArr to arr. Capacity of arr is increased if needed. */
void ogCArrayAppendArrayWithResize(ogCArray *arr, ogCArray *plusArr)
{
	ogCArrayEnsureExtraCapacity(arr, plusArr->num);
	ogCArrayAppendArray(arr, plusArr);
}

/** Removes all values from arr */
void ogCArrayRemoveAllValues(ogCArray *arr)
{
	arr->num = 0;
}

/** Removes value at specified index and pushes back all subsequent values.
 Behavior undefined if index outside [0, num-1].
 @since v0.99.4
 */
void ogCArrayRemoveValueAtIndex(ogCArray *arr, size_t index)
{
	for( size_t last = --arr->num; index < last; index++)
    {
		arr->arr[index] = arr->arr[index + 1];
    }
}

/** Removes value at specified index and fills the gap with the last value,
 thereby avoiding the need to push back subsequent values.
 Behavior undefined if index outside [0, num-1].
 @since v0.99.4
 */
void ogCArrayFastRemoveValueAtIndex(ogCArray *arr, size_t index)
{
	size_t last = --arr->num;
	arr->arr[index] = arr->arr[last];
}

/** Searches for the first oogurrence of value and removes it. If value is not found the function has no effect.
 @since v0.99.4
 */
void ogCArrayRemoveValue(ogCArray *arr, void* value)
{
	auto index = ogCArrayGetIndexOfValue(arr, value);
	if (index != -1)
    {
		ogCArrayRemoveValueAtIndex(arr, index);
    }
}

/** Removes from arr all values in minusArr. For each Value in minusArr, the first matching instance in arr will be removed.
 @since v0.99.4
 */
void ogCArrayRemoveArray(ogCArray *arr, ogCArray *minusArr)
{
	for(size_t i = 0; i < minusArr->num; i++)
    {
		ogCArrayRemoveValue(arr, minusArr->arr[i]);
    }
}

/** Removes from arr all values in minusArr. For each value in minusArr, all matching instances in arr will be removed.
 @since v0.99.4
 */
void ogCArrayFullRemoveArray(ogCArray *arr, ogCArray *minusArr)
{
	size_t back = 0;
	
	for(size_t i = 0; i < arr->num; i++)
    {
		if( ogCArrayContainsValue(minusArr, arr->arr[i]) ) 
        {
			back++;
		} 
        else
        {
			arr->arr[i - back] = arr->arr[i];
        }
	}
	
	arr->num -= back;
}

OG_END
