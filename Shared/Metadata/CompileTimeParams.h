//
// The Epoch Language Project
// Shared Library Code
//
// Interface for handling compile-time code execution
//
// Note that we sort of ended up abusing this interface a bit in the compiler,
// and use it for compile-time representation of all kinds of things, like parsed
// expressions and so on. While its chief purpose remains invocation of code to
// be executed at compile-time, its usage is overloaded somewhat.
//

#pragma once


// Dependencies
#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"
#include "Utility/Types/IDTypes.h"

#include <string>
#include <vector>


// Forward declarations
class FunctionSignature;


struct CompileTimeParameter
{
	// Constructor for convenience
	CompileTimeParameter(const std::wstring& name, VM::EpochTypeID type)
		: Name(name),
		  Type(type),
		  IsReference(false)
	{ }

	std::wstring Name;
	VM::EpochTypeID Type;

	union PayloadUnion
	{
		Integer32 IntegerValue;
		StringHandle LiteralStringHandleValue;
		BufferHandle BufferHandleValue;
		bool BooleanValue;
		Real32 RealValue;
	} Payload;

	std::wstring StringPayload;

	bool IsReference;
};


typedef std::vector<CompileTimeParameter> CompileTimeParameterVector;
