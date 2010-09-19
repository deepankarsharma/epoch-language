//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Central interface for retrieving information about types
//

#include "pch.h"

#include "Virtual Machine/TypeInfo.h"

#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/IDTypes.h"

#include <exception>


using namespace VM;


//
// Given a type ID, determine the amount of storage space needed
//
size_t VM::GetStorageSize(EpochTypeID type)
{
	switch(type)
	{
	case EpochType_Identifier:			return sizeof(StringHandle);

	case EpochType_Integer:				return sizeof(Integer32);
	case EpochType_String:				return sizeof(StringHandle);

	default:
		throw FatalException("Unable to determine the size of this variable/structure member");
	}
}
