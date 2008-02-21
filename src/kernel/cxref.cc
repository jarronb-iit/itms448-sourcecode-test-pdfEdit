/*
 * PDFedit - free program for PDF document manipulation.
 * Copyright (C) 2006, 2007, 2008  PDFedit team: Michal Hocko,
 *                                              Miroslav Jahoda,
 *                                              Jozef Misutka,
 *                                              Martin Petricek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in doc/LICENSE.GPL); if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307  USA
 *
 * Project is hosted on http://sourceforge.net/projects/pdfedit
 */
// vim:tabstop=4:shiftwidth=4:noexpandtab:textwidth=80
#include "kernel/static.h"
#include "kernel/cxref.h"
#include "utils/debug.h"
#include "kernel/factories.h"

using namespace pdfobjects;

void CXref::cleanUp()
{
using namespace debug;

	kernelPrintDbg(DBG_DBG, "");

	// deallocates changed storage
	// goes through all elemenents, erases all of them and deallocates value
	// returned from remove object (iterators are not invalidated by remove
	// method)
	kernelPrintDbg(DBG_DBG, "Deallocating changedStorage (size="<<changedStorage.size()<<")");
	ObjectStorage< ::Ref, ObjectEntry*, xpdf::RefComparator>::Iterator i;
	int index=0;
	for(i=changedStorage.begin(); i!=changedStorage.end(); ++i)
	{
		::Ref ref=i->first;
		ObjectEntry * entry=i->second;

		// check for entry
		if(!entry)
		{
			// this shouldn't happen
			kernelPrintDbg(DBG_ERR, ref<<" doesn't have entry. (index="<<index<<")");
			continue;
		}

		// check for entry object
		if(!entry->object)
		{
			// this shouldn't happen
			kernelPrintDbg(DBG_ERR, ref<<" entry doesn't have an object (index="<<index<<")");
			delete entry;
			i->second=NULL;
			continue;
		}

		kernelPrintDbg(DBG_DBG, "Deallocating entry for "<<ref<<" index="<<index<<")");
		xpdf::freeXpdfObject(entry->object);
		delete entry;
		i->second=NULL;
	}
	changedStorage.clear();
	kernelPrintDbg(DBG_DBG, "changedStorage cleaned up");

	kernelPrintDbg(DBG_DBG, "Cleaning newStorage");
	// newStorage doesn't need special entries deallocation
	newStorage.clear();
	kernelPrintDbg(DBG_DBG, "newStorage cleaned up");
}

CXref::~CXref()
{
using namespace debug;

	kernelPrintDbg(DBG_DBG, "");
	/* FIXME: uncoment when cache is ready.
	if(cache)
	{
		kernelPrintDbg(DBG_INFO, "Deallocating cache");
		delete cache;
	}
	*/
	
	kernelPrintDbg(DBG_INFO, "Deallocating internal structures");
	cleanUp();
	
	// XRef doesn't deallocate stream, so it has to be deallocated here
	delete str;
}

::Object * CXref::changeObject(::Ref ref, ::Object * instance)
{
using namespace debug;

	kernelPrintDbg(DBG_DBG, ref);
	assert(ref.num!=0);

	// discards from cache
	/* FIXME uncoment if cache is available
	if(cache)
		cache->discard(ref);
	*/

	// clones given object
	Object * clonedObject=instance->clone();
	if(!clonedObject)
	{
		// cloning has failed
		kernelPrintDbg(DBG_ERR, "Object can't be cloned.");
		throw NotImplementedException("clone failure.");
	}

	// searches in changedStorage
	// this is returned so it can be used for some
	// history information - value keeper is responsible for
	// deallocation
	ObjectEntry * changedEntry = changedStorage.get(ref);
	::Object * changed=(changedEntry)?changedEntry->object:NULL;
	
	// if changedEntry is NULL - object is changed for the first time
	// we have to allocate entry, otherwise, just sets new object
	// value and reset stored to false
	if(!changedEntry)
	{
		changedEntry=new ObjectEntry();
		kernelPrintDbg(DBG_DBG, "object is changed for the first time, creating changedEntry");
	}
	changedEntry->object=clonedObject;
	assert(ref.num!=0);

	// return value - original one - can be safely ignored, because either new 
	// entry is inserted or one from storage is changed directly
	changedStorage.put(ref, changedEntry);

	// object has been newly created, so we will set value in
	// the newStorage to true (so we know, that the value has
	// been set after initialization)
	if(newStorage.contains(ref))
	{
		newStorage.put(ref, INITIALIZED_REF);
		kernelPrintDbg(DBG_DBG, "newStorage entry changed to INITIALIZED_REF for "<<ref);
	}
	
	// returns old version
	return changed;
}


::Object * CXref::changeTrailer(const char * name, ::Object * value)
{
using namespace debug;

	kernelPrintDbg(DBG_DBG, "name="<<name<<" value type="<<value->getType());
	
	Dict * trailer = trailerDict.getDict(); 

	Object * clonedObject=value->clone();
	if(!clonedObject)
	{
		// cloning has failed
		kernelPrintDbg(DBG_ERR, "Object can't be cloned. Uses objNull instead");
		throw NotImplementedException("clone failure.");
	}
	char * key=strdup(name);
	::Object * prev = trailer->update(key, clonedObject);

	// update doesn't store key if key, value has been already in the 
	// dictionary
	if(prev)
		free(key);

	return prev;
}

::Ref CXref::reserveRef()
{
using namespace debug;

	int i=1;
	int num=-1, gen=0;

	kernelPrintDbg(DBG_DBG, "");
	
	// goes through entries array in XRef class (xref entries)
	// and reuses first free entry with its gen number.
	// Considers just first XRef::getNumObjects because entries array
	// is allocated by blocks and so there are entries which are marked 
	// as free but they are not realy removed objects.
	int objectCount=0, xrefCount=XRef::getNumObjects();
	for(; i<size && i<MAXOBJNUM && objectCount<xrefCount; ++i)
	{
		if(entries[i].type!=xrefEntryFree)
		{
			++objectCount;
			continue;
		}

		// reference is never reused if generation number is MAXOBJGEN
		// according specification
		if(entries[i].gen>=MAXOBJGEN)
		{
			kernelPrintDbg(DBG_DBG, "Entry ["<<i<<", "<<entries[i].gen<<"] can't be reused. MAXOBJGEN reached.");
			continue;
		}

		// entry is marked as free, we can reuse it only if it is not in
		// reservation process - not in newStorage
		Ref ref={i, entries[i].gen};
		if(newStorage.contains(ref))
		{
			continue;
		}

		// Entry is free and not used yet. 
		if(ref.gen)
		{
			kernelPrintDbg(DBG_DBG, "Reusing entry "<<ref);
		}
		else
		{
			// gen is 0, so entry couldn't have been removed - so say that it is
			// new entry
			kernelPrintDbg(DBG_DBG, "Using new entry "<<ref);
		}
			
		num=ref.num;
		gen=ref.gen;
		break;
	}

	// no entry for reuse, so new has to be used
	if(num==-1)
	{
		// checks if num, gen is not in newStorage and if yes,
		// use num+1
		for(;i<MAXOBJNUM; ++i)
		{
			Ref ref={i, 0};
			if(!newStorage.contains(ref))
			{
				num=i;
				break;
			}
		}

		if(num==-1)
		{
			// all object numbers are used, no more indirect objects
			// can be created
			throw IndirectObjectsExhausted();
		}

		// ok, we have new num and gen is 0, because object is new
		gen=0;
		kernelPrintDbg(DBG_DBG, "Using new entry ["<<num<<", "<<gen<<"]");
	}
	
	// Registers newly created reference to the newStorage.
	// Flag is set to false now and this is changed only if
	// initialized value is overwritten by change method.
	::Ref objRef={num, gen};
	newStorage.put(objRef, RESERVED_REF);
	kernelPrintDbg(DBG_INFO, objRef<<" registered to newStorage");

	return objRef;
}

::Object * CXref::createObject(::ObjType type, ::Ref * ref)
{
using namespace debug;

	kernelPrintDbg(DBG_DBG, "type="<<type);
	// reserves new reference for object 
	::Ref objRef=reserveRef();
	
	// it's not possible to create indirect reference value or
	// any other internaly (by xpdf) used object types
	// FIXME
	//if(type==objRef || type==objError || type==objEOF || type==objNone)
	//	return 0;

	::Object * obj=XPdfObjectFactory::getInstance();
	
	// initialize according type
	switch(type)
	{
		case objBool:
			obj->initBool(false);
			break;
		case objInt:
			obj->initInt(0);
			break;
		case objReal:
			obj->initReal(0);
			break;
		case objString:
			obj->initString(NULL);
			break;
		case objName:
			obj->initName(NULL);
			break;
		case objArray:
			obj->initArray(this);
			break;
		case objDict:
			obj->initDict(this);
			break;
		case objStream:
			obj->initStream(NULL);
			break;
		case objCmd:
			obj->initCmd(NULL);
			break;
		case objNull:
			obj->initNull();
			break;
		default:
			// TODO assert

		break;
	}
	

	// sets reference parameter if not null
	if(ref)
		*ref=objRef;

	kernelPrintDbg(DBG_INFO, "new object created and initialized");
	return obj;
}

RefState CXref::knowsRef(::Ref& ref)
{
using namespace debug;

	kernelPrintDbg(DBG_DBG, "");

	// checks newly created object only with true flag
	// not found returns 0, so it's ok
	::RefState state;
	if((state=newStorage.get(ref))>UNUSED_REF)
	{
		kernelPrintDbg(DBG_DBG, "Reference is in newStorage. state="<<state);
		return state;
	}

	kernelPrintDbg(DBG_DBG, "Reference is not in newStorage. Trying XRef.");
	// object has to be in in XRef
	state=XRef::knowsRef(ref);
	kernelPrintDbg(DBG_DBG, "Reference state in XRef is "<<state);
	return state;
}

bool CXref::typeSafe(::Object * obj1, ::Object * obj2)
{
using namespace debug;

	if(!obj1 || !obj2)
	{
		if(!obj1)
			kernelPrintDbg(DBG_ERR, "obj1 is null");
		
		if(!obj2)
			kernelPrintDbg(DBG_ERR, "obj2 is null");
		return false;
	}
	 
	// object for dereferenced  values - we assume, it's not indirect
	::Object dObj1=*obj1, dObj2=*obj2;	
	// types for direct values.
	::ObjType type1=obj1->getType(), type2=obj2->getType();
	
	// checks indirect
	if(type1==objRef)
	{
		kernelPrintDbg(DBG_DBG, "obj1 is reference - need to dereference");
		fetch(dObj1.getRef().num, dObj1.getRef().gen, &dObj1);
		type1=dObj1.getType();
		kernelPrintDbg(DBG_DBG, "obj1 target type="<< type1);
		// has to free object
		dObj1.free();
	}
	if(type2==objRef)
	{
		kernelPrintDbg(DBG_DBG, "obj2 is reference - need to dereference");
		fetch(dObj2.getRef().num, dObj2.getRef().gen, &dObj2);
		type2=dObj2.getType();
		kernelPrintDbg(DBG_DBG, "obj2 target type="<< type2);
		// has to free object
		dObj2.free();
	}

	// now we have direct values' types
	bool ret=type1==type2;
	
	// if these types are not same, one additional situation may occure:
	// obj1 is objRef (it is indirect) and dereferenced object is objNull 
	// (it is not present in pdf). In such situation, type2
	// can be everything - other direction (obj1 is whatever and obj2 is objNull
	// is not allowed)
	if(!ret && (obj1->getType() == objRef && type1 == objNull))
		ret=true;

	kernelPrintDbg(DBG_DBG, "typeSafe result="<<ret);
	return ret;
}

::Object * CXref::getTrailerEntry(char * name)
{
using namespace debug;

	kernelPrintDbg(DBG_DBG, "name="<<name);
	Dict * trailer = trailerDict.getDict();

	// lookupNF doesn't create deep copy, so 
	// we have to get value and then make deep copy
	// calling clone method. To keep clean reference counting
	// obj has to be freed
	::Object obj;
	trailer->lookupNF(name, &obj);

	::Object * retValue=obj.clone();
	obj.free();
	if(!retValue)
	{
		// cloning has failed
		kernelPrintDbg(DBG_ERR, "Object can't be cloned. Uses objNull instead");
		throw NotImplementedException("clone failure.");
	}

	return retValue;
}

::Object *CXref::getDocInfo(::Object *obj)
{
using namespace debug;

	kernelPrintDbg(DBG_DBG, "");
	// gets object
	::Object docObj;
	XRef::getDocInfo(&docObj);

	// creates deep copy and frees object from getDocInfo
	// and initialize parameter from cloned value
	::Object * retObj=docObj.clone();
	docObj.free();
	if(!retObj)
	{
		// cloning has failed
		kernelPrintDbg(DBG_ERR, "Object can't be cloned. Uses objNull instead");
		throw NotImplementedException("clone failure.");
	}
	*obj=*retObj;

	return retObj;
}

::Object *CXref::getDocInfoNF(::Object *obj)
{
using namespace debug;

	kernelPrintDbg(DBG_DBG, "");
	// gets object
	::Object docObj;
	XRef::getDocInfoNF(&docObj);

	// creates deep copy and frees object (because getDocInfoNF 
	// uses copy method) from getDocInfo
	// and initialize parameter from cloned value
	::Object * retObj=docObj.clone();
	docObj.free();
	if(!retObj)
	{
		// cloning has failed
		kernelPrintDbg(DBG_ERR, "Object can't be cloned. Uses objNull instead");
		throw NotImplementedException("clone failure.");
	}
	// shallow copy of the content (deep copied)
	*obj=*retObj;

	return retObj;
}

::Object * CXref::fetch(int num, int gen, ::Object *obj)
{
using namespace debug;

	kernelPrintDbg(DBG_DBG, "num="<<num<<" gen="<<gen);
	
	::Ref ref={num, gen};
	
	// tries to use cache
	// TODO uncomment when cache is ready
	/*
	if(cache)
	{
		kernelPrintDbg(DBG_INFO, "[num="<<num<<" gen"<<gen<<"] is cached");
	}
	*/
	
	ObjectEntry * entry=changedStorage.get(ref);
	if(entry)
	{
		kernelPrintDbg(DBG_INFO, ref<<" is changed - using changedStorage");
		
		// object has been changed
		// this clone never fails, because it had to be cloned before storing to
		// changedStorage
		::Object * object=entry->object;
		if(!object)
		{
			// this shouldn't not happen
			// TODO handle - exception should be called
			kernelPrintDbg(DBG_CRIT, "Changed object is NULL!");
			return obj;
		}
		::Object * deepCopy=entry->object->clone();

		// shallow copy of content
		// content is deep copy of found object, so
		// this doesn't affect our ::Object in changedStorage
		*obj=*deepCopy;
		
		// dellocate deepCopy - content is kept
		gfree(deepCopy); 
		return obj;
	}

	// delegates to original implementation
	kernelPrintDbg(DBG_INFO, ref<<" is not changed - using Xref");
	Object tmpObj;
	XRef::fetch(num, gen, &tmpObj);

	// clones fetched object
	// this has to be done because return value may be stream and we want to
	// prevent direct changing of the stream
	Object * cloneObj=tmpObj.clone();
	// deallocates XRef returned object content
	tmpObj.free();
	if(!cloneObj)
	{
		// cloning has failed
		kernelPrintDbg(DBG_ERR, "Object can't be cloned. Uses objNull instead");
		throw NotImplementedException("clone failure.");
	}

	// shallow copy of cloned value and
	// deallocates coned value, but keeps content
	*obj=*cloneObj;
	gfree(cloneObj);

	// if object is not null, caches object's deep copy
	/* FIXME uncoment, when cache is ready
	if(cache && obj->getType()!=objNull)
	{
		kernelPrintDbg(DBG_DBG, "Caching object "[num="<<num<<" gen="<<gen<<"]");
		cache->put(ref, obj->clone());
	}
	*/   

	return obj;
}

int CXref::getNumObjects() 
{ 
using namespace debug;

	kernelPrintDbg(DBG_DBG, "");

	size_t newSize=0;
	ObjectStorage< ::Ref, RefState, xpdf::RefComparator>::Iterator begin, i;

	for(i=newStorage.begin(); i!=newStorage.end(); ++i)
		if(i->second==INITIALIZED_REF)
			++newSize;

	kernelPrintDbg(DBG_INFO, "original objects count="<<XRef::getNumObjects()<<" newly created="<<newSize);
	return XRef::getNumObjects() + newSize;
}


void CXref::reopen(size_t xrefOff)
{
using namespace debug;

	kernelPrintDbg(DBG_DBG, "");

	// clears all object storages
	kernelPrintDbg(DBG_INFO, "Destroying CXref internals");
	cleanUp();

	// clears XRef internals and forces to fill them again
	kernelPrintDbg(DBG_INFO, "Destroing XRef internals");
	XRef::destroyInternals();
	kernelPrintDbg(DBG_INFO, "Initializes XRef internals");
	XRef::initInternals(xrefOff);

	// sets lastXRefPos to xrefOff, because initRevisionSpecific doesn't do it
	lastXRefPos=xrefOff;
	kernelPrintDbg(DBG_DBG, "New lastXRefPos value="<<lastXRefPos);
}
