//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a function
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"

#include "Compiler/Exceptions.h"


using namespace IRSemantics;


//
// Destruct and clean up a function
//
Function::~Function()
{
	for(std::vector<Param>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		delete iter->Parameter;

	delete Code;
	delete Return;
}

//
// Add a parameter to a function's signature
//
void Function::AddParameter(StringHandle name, FunctionParam* param)
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
		{
			delete param;
			throw std::runtime_error("Duplicate function parameter name");		// TODO - this should not be an exception
		}
	}

	Parameters.push_back(Param(name, param));
}

//
// Set a function's return expression
//
void Function::SetReturnExpression(IRSemantics::Expression* expression)
{
	delete Return;
	Return = expression;
}

VM::EpochTypeID Function::GetReturnType(const Program& program) const
{
	if(Return)
		return Return->GetEpochType(program);

	return VM::EpochType_Void;
}

//
// Set a function's code body
//
void Function::SetCode(CodeBlock* code)
{
	delete Code;
	Code = code;
}

bool Function::IsParameterLocalVariable(StringHandle name) const
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
			return iter->Parameter->IsLocalVariable();
	}

	// TODO - ensure this exception cannot be thrown by simply malforming code
	throw InternalException("Provided string handle does not correspond to a parameter of this function");
}

bool Function::IsParameterReference(StringHandle name) const
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
			return iter->Parameter->IsReference();
	}

	// TODO - ensure this exception cannot be thrown by simply malforming code
	throw InternalException("Provided string handle does not correspond to a parameter of this function");
}

VM::EpochTypeID Function::GetParameterType(StringHandle name, const IRSemantics::Program& program) const
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
			return iter->Parameter->GetParamType(program);
	}

	// TODO - ensure this exception cannot be thrown by simply malforming code
	throw InternalException("Provided string handle does not correspond to a parameter of this function");
}

std::vector<StringHandle> Function::GetParameterNames() const
{
	std::vector<StringHandle> ret;
	ret.reserve(Parameters.size());

	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		ret.push_back(iter->Name);

	return ret;
}


//
// Validate a function definition
//
bool Function::Validate(const IRSemantics::Program& program) const
{
	bool valid = true;

	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!iter->Parameter->Validate(program))
			valid = false;
	}

	if(Return)
	{
		if(!Return->Validate(program))
			valid = false;
	}

	if(Code)
	{
		if(!Code->Validate(program))
			valid = false;
	}

	return valid;
}


bool Function::CompileTimeCodeExecution(Program& program)
{
	if(Return)
	{
		if(!Code)
			return false;

		if(!Return->CompileTimeCodeExecution(program, *Code, true))
			return false;
	}

	if(!Code)
		return true;

	return Code->CompileTimeCodeExecution(program);
}


bool Function::TypeInference(Program& program, InferenceContext& context)
{
	if(InferenceDone)
		return true;

	InferenceDone = true;

	if(!Code)
		return true;

	if(Return)
	{
		InferenceContext newcontext(Name, InferenceContext::CONTEXT_FUNCTION_RETURN);
		if(!Return->TypeInference(program, *Code, newcontext, 0))
			return false;
	}

	InferenceContext newcontext(Name, InferenceContext::CONTEXT_FUNCTION);
	return Code->TypeInference(program, newcontext);
}



//
// Validate a named function parameter
//
bool FunctionParamNamed::Validate(const IRSemantics::Program& program) const
{
	return (program.LookupType(MyType) != VM::EpochType_Error);
}

VM::EpochTypeID FunctionParamNamed::GetParamType(const IRSemantics::Program& program) const
{
	return program.LookupType(MyType);
}

//
// Validate a higher order function parameter
//
bool FunctionParamFuncRef::Validate(const IRSemantics::Program& program) const
{
	bool valid = true;

	if(program.LookupType(ReturnType) == VM::EpochType_Error)
		valid = false;

	for(std::vector<StringHandle>::const_iterator iter = ParamTypes.begin(); iter != ParamTypes.end(); ++iter)
	{
		if(program.LookupType(*iter) == VM::EpochType_Error)
			valid = false;
	}

	return valid;
}

//
// Validate an expression function parameter
//
bool FunctionParamExpression::Validate(const IRSemantics::Program& program) const
{
	if(!MyExpression)
		return false;

	return MyExpression->Validate(program);
}

//
// Destruct and clean up an expression function parameter
//
FunctionParamExpression::~FunctionParamExpression()
{
	delete MyExpression;
}

VM::EpochTypeID FunctionParamExpression::GetParamType(const IRSemantics::Program& program) const
{
	if(!MyExpression)
		return VM::EpochType_Error;

	return MyExpression->GetEpochType(program);
}
