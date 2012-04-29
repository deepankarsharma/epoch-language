//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for containing a lexical scope and its contents
//

#include "pch.h"

#include "Metadata/ActiveScope.h"
#include "Metadata/ScopeDescription.h"

#include "Virtual Machine/VirtualMachine.h"
#include "Virtual Machine/TypeInfo.h"

#include "Utility/Memory/Stack.h"

#include "Utility/Types/RealTypes.h"


//
// Attach the parameters defined in the current scope to a given stack space
//
// Each variable within the scope which has its origin specified as an entity parameter is
// bound to a storage location within the given stack space. The stack space is assumed to
// contain all of the parameters to the entity in order as per the calling convention.
//
void ActiveScope::BindParametersToStack(const VM::ExecutionContext& context)
{
	char* stackpointer = reinterpret_cast<char*>(context.State.Stack.GetCurrentTopOfStack());

	for(ScopeDescription::VariableVector::const_reverse_iterator iter = OriginalScope.Variables.rbegin(); iter != OriginalScope.Variables.rend(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_PARAMETER)
		{
			if(iter->IsReference)
			{
				void* targetstorage = *reinterpret_cast<void**>(stackpointer);
				stackpointer += sizeof(void*);
				VM::EpochTypeID targettype = *reinterpret_cast<VM::EpochTypeID*>(stackpointer);
				stackpointer += sizeof(VM::EpochTypeID);
				BindReference(iter->IdentifierHandle, targetstorage, targettype);
			}
			else
			{
				VariableStorageLocations[iter->IdentifierHandle] = stackpointer;
				stackpointer += VM::GetStorageSize(iter->Type);
			}
		}
	}
}

//
// Allocate stack space for all local variables (including return value holders) on the given stack
//
void ActiveScope::PushLocalsOntoStack(VM::ExecutionContext& context)
{
	for(ScopeDescription::VariableVector::const_iterator iter = OriginalScope.Variables.begin(); iter != OriginalScope.Variables.end(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_LOCAL || iter->Origin == VARIABLE_ORIGIN_RETURN)
		{
			size_t size = VM::GetStorageSize(iter->Type);
			context.State.Stack.Push(size);

			VariableStorageLocations[iter->IdentifierHandle] = context.State.Stack.GetCurrentTopOfStack();
		}
	}
}

//
// Pop a stack so as to reset it after holding parameters and local variables from the current scope
//
void ActiveScope::PopScopeOffStack(VM::ExecutionContext& context)
{
	size_t usedspace = 0;

	for(ScopeDescription::VariableVector::const_iterator iter = OriginalScope.Variables.begin(); iter != OriginalScope.Variables.end(); ++iter)
	{
		if(iter->IsReference)
			usedspace += sizeof(StringHandle) + sizeof(void*);
		else
			usedspace += VM::GetStorageSize(iter->Type);
	}

	context.State.Stack.Pop(usedspace);
}

//
// Write the topmost stack entry into a given arbitrary storage location
//
void ActiveScope::WriteFromStack(void* targetstorage, VM::EpochTypeID targettype, StackSpace& stack)
{
	switch(targettype)
	{
	case VM::EpochType_Integer:
		Write(targetstorage, stack.PopValue<Integer32>());
		break;

	case VM::EpochType_Integer16:
		Write(targetstorage, stack.PopValue<Integer16>());
		break;

	case VM::EpochType_String:
		Write(targetstorage, stack.PopValue<StringHandle>());
		break;

	case VM::EpochType_Boolean:
		Write(targetstorage, stack.PopValue<bool>());
		break;

	case VM::EpochType_Real:
		Write(targetstorage, stack.PopValue<Real32>());
		break;

	case VM::EpochType_Buffer:
		Write(targetstorage, stack.PopValue<BufferHandle>());
		break;

	default:
		if(VM::GetTypeFamily(targettype) != VM::EpochTypeFamily_Structure)
			throw NotImplementedException("Unsupported type in ActiveScope::WriteFromStack");
		
		Write(targetstorage, stack.PopValue<StructureHandle>());
		break;
	}
}


//
// Internal helper to minimize code redundancy
//
namespace
{
	template <typename T>
	void DoTypedPush(StackSpace& stack, void* targetstorage)
	{
		T* value = reinterpret_cast<T*>(targetstorage);
		stack.PushValue<T>(*value);
	}
}


//
// Push data from an arbitrary location onto the stack
//
void ActiveScope::PushOntoStack(void* targetstorage, VM::EpochTypeID targettype, StackSpace& stack) const
{
	switch(targettype)
	{
	case VM::EpochType_Integer:			DoTypedPush<Integer32>(stack, targetstorage);		break;
	case VM::EpochType_Integer16:		DoTypedPush<Integer16>(stack, targetstorage);		break;
	case VM::EpochType_String:			DoTypedPush<StringHandle>(stack, targetstorage);	break;
	case VM::EpochType_Boolean:			DoTypedPush<bool>(stack, targetstorage);			break;
	case VM::EpochType_Real:			DoTypedPush<Real32>(stack, targetstorage);			break;
	case VM::EpochType_Buffer:			DoTypedPush<BufferHandle>(stack, targetstorage);	break;
	case VM::EpochType_Function:		DoTypedPush<StringHandle>(stack, targetstorage);	break;
	case VM::EpochType_Identifier:		DoTypedPush<StringHandle>(stack, targetstorage);	break;
	case VM::EpochType_Nothing:			stack.Push(0);										break;
	default:
		if(VM::GetTypeFamily(targettype) != VM::EpochTypeFamily_Structure)
			throw NotImplementedException("Unsupported data type in ActiveScope::PushOntoStack");

		DoTypedPush<StructureHandle>(stack, targetstorage);
		break;
	}
}

//
// Push a variable's value onto the top of the given stack
//
void ActiveScope::PushOntoStack(StringHandle variableid, StackSpace& stack) const
{
	if(OriginalScope.IsReferenceByID(variableid))
	{
		PushOntoStackDeref(variableid, stack);
		return;
	}

	PushOntoStack(GetVariableStorageLocation(variableid), OriginalScope.GetVariableTypeByID(variableid), stack);
}

//
// Dereference a variable and push the resultant value onto the stack
//
void ActiveScope::PushOntoStackDeref(StringHandle variableid, StackSpace& stack) const
{
	ReferenceBindingMap::const_iterator iter = BoundReferences.find(variableid);
	if(iter == BoundReferences.end())
		throw FatalException("Unbound reference");

	PushOntoStack(iter->second.first, iter->second.second, stack);
}

//
// Retrieve the storage location in memory of the given variable
//
// This may reside on the stack or the freestore or even be a reference to another
// variable's storage position, depending on the context. In general it is not safe
// to assume that a variable resides in any one particular location.
//
void* ActiveScope::GetVariableStorageLocation(StringHandle variableid) const
{
	const ActiveScope* thisptr = this;
	while(thisptr)
	{
		std::map<StringHandle, void*>::const_iterator iter = thisptr->VariableStorageLocations.find(variableid);
		if(iter != thisptr->VariableStorageLocations.end())
			return iter->second;

		thisptr = thisptr->ParentScope;
	}

	throw InvalidIdentifierException("Variable ID has not been mapped to a storage location in this scope");
}

//
// Copy a variable's value into the provided virtual machine register
//
void ActiveScope::CopyToRegister(StringHandle variableid, Register& targetregister) const
{
	VM::EpochTypeID variabletype = OriginalScope.GetVariableTypeByID(variableid);
	switch(variabletype)
	{
	case VM::EpochType_Integer:
		{
			Integer32* value = reinterpret_cast<Integer32*>(GetVariableStorageLocation(variableid));
			targetregister.Set(*value);
		}
		break;

	case VM::EpochType_Integer16:
		{
			Integer16* value = reinterpret_cast<Integer16*>(GetVariableStorageLocation(variableid));
			targetregister.Set(*value);
		}
		break;

	case VM::EpochType_Identifier:
	case VM::EpochType_String:
		{
			StringHandle* value = reinterpret_cast<StringHandle*>(GetVariableStorageLocation(variableid));
			targetregister.SetString(*value);
		}
		break;

	case VM::EpochType_Boolean:
		{
			bool* value = reinterpret_cast<bool*>(GetVariableStorageLocation(variableid));
			targetregister.Set(*value);
		}
		break;

	case VM::EpochType_Real:
		{
			Real32* value = reinterpret_cast<Real32*>(GetVariableStorageLocation(variableid));
			targetregister.Set(*value);
		}
		break;

	case VM::EpochType_Buffer:
		{
			BufferHandle* value = reinterpret_cast<BufferHandle*>(GetVariableStorageLocation(variableid));
			targetregister.SetBuffer(*value);
		}
		break;

	default:
		{
			if(VM::GetTypeFamily(variabletype) != VM::EpochTypeFamily_Structure)
				throw FatalException("Unsupported type when assigning to register");

			StructureHandle* value = reinterpret_cast<StructureHandle*>(GetVariableStorageLocation(variableid));
			targetregister.SetStructure(*value, OriginalScope.GetVariableTypeByID(variableid));
		}
		break;
	}
}

//
// Determine if the current scope contains a variable bound to an entity return value
//
bool ActiveScope::HasReturnVariable() const
{
	for(ScopeDescription::VariableVector::const_iterator iter = OriginalScope.Variables.begin(); iter != OriginalScope.Variables.end(); ++iter)
	{
		if(iter->Origin == VARIABLE_ORIGIN_RETURN)
			return true;
	}

	return false;
}


//
// Bind a reference variable to a target
//
void ActiveScope::BindReference(StringHandle referencename, void* targetstorage, VM::EpochTypeID targettype)
{
	BoundReferences[referencename].first = targetstorage;
	BoundReferences[referencename].second = targettype;
}

//
// Retrieve the storage location pointed to by a reference variable
//
void* ActiveScope::GetReferenceTarget(StringHandle referencename) const
{
	ReferenceBindingMap::const_iterator iter = BoundReferences.find(referencename);
	if(iter == BoundReferences.end())
		throw FatalException("Unbound reference");

	return iter->second.first;
}

//
// Retrieve the underlying type of a reference variable
//
VM::EpochTypeID ActiveScope::GetReferenceType(StringHandle referencename) const
{
	ReferenceBindingMap::const_iterator iter = BoundReferences.find(referencename);
	if(iter == BoundReferences.end())
		throw FatalException("Unbound reference");

	return iter->second.second;
}

