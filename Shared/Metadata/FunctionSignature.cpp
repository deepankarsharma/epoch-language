//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for describing function interfaces
//

#include "pch.h"

#include "Metadata/FunctionSignature.h"

#include "Utility/StringPool.h"


//
// Construct and initialize a function signature wrapper
//
FunctionSignature::FunctionSignature()
	: ReturnType(Metadata::EpochType_Void)
{
}

//
// Add a parameter with the given name and data type to the function signature
//
void FunctionSignature::AddParameter(const std::wstring& name, Metadata::EpochTypeID type)
{
	Parameters.push_back(CompileTimeParameter(name, type));
	FunctionSignatures.push_back(FunctionSignature());
}

void FunctionSignature::PrependParameter(const std::wstring& name, Metadata::EpochTypeID type)
{
	Parameters.insert(Parameters.begin(), CompileTimeParameter(name, type));
	FunctionSignatures.insert(FunctionSignatures.begin(), FunctionSignature());
}

//
// Add a parameter that is used in pattern matching which has an integer literal type
//
void FunctionSignature::AddPatternMatchedParameter(Integer32 literalvalue)
{
	CompileTimeParameter ctparam(L"@@patternmatched", Metadata::EpochType_Integer);
	ctparam.Payload.IntegerValue = literalvalue;
	ctparam.HasPayload = true;
	Parameters.push_back(ctparam);
	FunctionSignatures.push_back(FunctionSignature());
}

//
// Add a parameter that is used in pattern matching which has a type of identifier
//
void FunctionSignature::AddPatternMatchedParameterIdentifier(StringHandle identifier)
{
	CompileTimeParameter ctparam(L"@@patternmatched", Metadata::EpochType_Identifier);
	ctparam.Payload.LiteralStringHandleValue = identifier;
	ctparam.HasPayload = true;
	Parameters.push_back(ctparam);
	FunctionSignatures.push_back(FunctionSignature());
}

//
// Set the return type of the function
//
void FunctionSignature::SetReturnType(Metadata::EpochTypeID type)
{
	ReturnType = type;
}

//
// Retrieve the parameter at the given index
//
const CompileTimeParameter& FunctionSignature::GetParameter(size_t index) const
{
	return Parameters[index];
}

//
// Retrieve the parameter with the given name
//
const CompileTimeParameter& FunctionSignature::GetParameter(const std::wstring& name) const
{
	return GetParameter(FindParameter(name));
}

//
// Locate the index of the parameter with the given name
//
size_t FunctionSignature::FindParameter(const std::wstring& name) const
{
	for(size_t i = 0; i < Parameters.size(); ++i)
	{
		if(Parameters[i].Name == name)
			return i;
	}

	throw FatalException("Invalid parameter name");
}

//
// Retrieve the function signature at the given parameter index (for higher-order functions)
//
const FunctionSignature& FunctionSignature::GetFunctionSignature(size_t index) const
{
	return FunctionSignatures[index];
}

//
// Set the value of the function signature at the given parameter index (for higher-order functions)
//
void FunctionSignature::SetFunctionSignature(size_t index, const FunctionSignature& signature)
{
	FunctionSignatures[index] = signature;
}

//
// Determine if two function signatures match
//
bool FunctionSignature::Matches(const FunctionSignature& rhs) const
{
	if(ReturnType != rhs.ReturnType && ReturnType != Metadata::EpochType_Infer && rhs.ReturnType != Metadata::EpochType_Infer)
		return false;

	if(Parameters.size() != rhs.Parameters.size())
		return false;

	for(size_t i = 0; i < Parameters.size(); ++i)
	{
		if(Parameters[i].Type != rhs.Parameters[i].Type && Parameters[i].Type != Metadata::EpochType_Infer && rhs.Parameters[i].Type != Metadata::EpochType_Infer)
			return false;

		if(Metadata::GetTypeFamily(Parameters[i].Type) == Metadata::EpochTypeFamily_Function)
		{
			if(!FunctionSignatures[i].Matches(rhs.FunctionSignatures[i]))
				return false;
		}

		if(Parameters[i].HasPayload != rhs.Parameters[i].HasPayload)
			return false;

		if(Parameters[i].HasPayload)
		{
			switch(Parameters[i].Type)
			{
			case Metadata::EpochType_Boolean:
				if(Parameters[i].Payload.BooleanValue != rhs.Parameters[i].Payload.BooleanValue)
					return false;
				break;

			case Metadata::EpochType_Integer:
				if(Parameters[i].Payload.IntegerValue != rhs.Parameters[i].Payload.IntegerValue)
					return false;
				break;

			case Metadata::EpochType_String:
				if(Parameters[i].StringPayload != rhs.Parameters[i].StringPayload)
					return false;
				break;

			case Metadata::EpochType_Real:
				if(Parameters[i].Payload.RealValue != rhs.Parameters[i].Payload.RealValue)
					return false;
				break;

			default:
				throw std::runtime_error("Not implemented");
			}
		}
	}

	return true;
}

bool FunctionSignature::MatchesDynamicPattern(const FunctionSignature& rhs) const
{
	if(ReturnType != rhs.ReturnType && ReturnType != Metadata::EpochType_Infer && rhs.ReturnType != Metadata::EpochType_Infer)
		return false;

	if(Parameters.size() != rhs.Parameters.size())
		return false;

	for(size_t i = 0; i < Parameters.size(); ++i)
	{
		if(Parameters[i].Type != rhs.Parameters[i].Type && Parameters[i].Type != Metadata::EpochType_Infer && rhs.Parameters[i].Type != Metadata::EpochType_Infer)
			return false;

		if(Metadata::GetTypeFamily(Parameters[i].Type) == Metadata::EpochTypeFamily_Function)
		{
			if(!FunctionSignatures[i].Matches(rhs.FunctionSignatures[i]))
				return false;
		}

		if(rhs.Parameters[i].HasPayload)
			return false;
	}

	return true;
}
