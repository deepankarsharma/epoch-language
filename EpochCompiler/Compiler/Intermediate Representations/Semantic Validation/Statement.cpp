//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class for representing statements
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Statement.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"

#include "Compiler/Intermediate Representations/Semantic Validation/InferenceContext.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"

#include "Compiler/Session.h"
#include "Compiler/Exceptions.h"

#include "User Interface/Output.h"


using namespace IRSemantics;


Statement::Statement(StringHandle name, const AST::IdentifierT& identifier)
	: Name(name),
	  MyType(VM::EpochType_Error),
	  OriginalIdentifier(identifier)
{
}

Statement::~Statement()
{
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		delete *iter;
}

void Statement::AddParameter(Expression* expression)
{
	Parameters.push_back(expression);
}


bool Statement::Validate(const Program& program) const
{
	bool valid = true;

	for(std::vector<Expression*>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->Validate(program))
			valid = false;
	}

	valid = valid && (MyType != VM::EpochType_Infer) && (MyType != VM::EpochType_Error);
	return valid;
}

bool Statement::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors)
{
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(program, activescope, inreturnexpr, errors))
			return false;
	}

	FunctionCompileHelperTable::const_iterator fchiter = program.Session.InfoTable.FunctionHelpers->find(Name);
	if(fchiter != program.Session.InfoTable.FunctionHelpers->end())
		fchiter->second(*this, program, activescope, inreturnexpr);

	return true;
}

bool Statement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, size_t index, CompileErrors& errors)
{
	if(MyType != VM::EpochType_Error)
		return (MyType != VM::EpochType_Infer);

	InferenceContext newcontext(Name, InferenceContext::CONTEXT_STATEMENT);
	newcontext.FunctionName = context.FunctionName;

	errors.SetContext(OriginalIdentifier);

	switch(context.State)
	{
	case InferenceContext::CONTEXT_CODE_BLOCK:
	case InferenceContext::CONTEXT_EXPRESSION:
	case InferenceContext::CONTEXT_FUNCTION_RETURN:
		newcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(Name, *activescope.GetScope(), context.FunctionName, errors));
		newcontext.ExpectedSignatures.push_back(program.GetExpectedSignaturesForStatement(Name, *activescope.GetScope(), context.FunctionName, errors));
		break;

	case InferenceContext::CONTEXT_STATEMENT:
		newcontext.ExpectedTypes.push_back(program.GetExpectedTypesForStatement(context.ContextName, *activescope.GetScope(), context.FunctionName, errors));
		newcontext.ExpectedSignatures.push_back(program.GetExpectedSignaturesForStatement(context.ContextName, *activescope.GetScope(), context.FunctionName, errors));
		break;

	default:
		//
		// This is a parser failure or the result of incomplete
		// language feature implementation. The statement node
		// in the AST has occurred in a context in which type
		// inference cannot be performed because the context is
		// not recognized by one of the above switch cases.
		//
		// Check to ensure the statement node is placed in a
		// valid location in the AST, that AST traversal is not
		// broken in some way, and that any new/partially done
		// language features are handled appropriately above.
		//
		throw InternalException("Statement type inference failure - unrecognized context");
	}

	if(context.State == InferenceContext::CONTEXT_FUNCTION_RETURN)
		MyType = program.LookupType(Name);

	size_t i = 0;
	for(std::vector<Expression*>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!(*iter)->TypeInference(program, activescope, newcontext, i, errors))
			return false;

		++i;
	}

	if(context.State != InferenceContext::CONTEXT_FUNCTION_RETURN)
	{
		if(program.HasFunction(Name))
		{
			InferenceContext funccontext(0, InferenceContext::CONTEXT_GLOBAL);
			unsigned overloadcount = program.GetNumFunctionOverloads(Name);
			for(unsigned i = 0; i < overloadcount; ++i)
			{
				StringHandle overloadname = program.GetFunctionOverloadName(Name, i);
				Function* func = program.GetFunctions().find(overloadname)->second;
				func->TypeInference(program, funccontext, errors);

				if(!context.ExpectedTypes.empty())
				{
					bool matchedreturn = false;
					const InferenceContext::PossibleParameterTypes& possibles = context.ExpectedTypes.back();
					for(size_t j = 0; j < possibles.size(); ++j)
					{
						if(possibles[j][index] == func->GetReturnType(program))
						{
							matchedreturn = true;
							break;
						}
					}

					if(!matchedreturn)
						continue;
				}
				
				if(func->GetNumParameters() == Parameters.size())
				{
					FunctionSignature signature = func->GetFunctionSignature(program);
					bool match = true;
					for(size_t j = 0; j < Parameters.size(); ++j)
					{
						VM::EpochTypeID thisparamtype = func->GetParameterTypeByIndex(j, program);
						if(thisparamtype != Parameters[j]->GetEpochType(program))
						{
							match = false;
							break;
						}

						if(signature.GetParameter(j).HasPayload)
						{
							if(Parameters[j]->GetAtoms().size() == 1)
							{
								if(!func->PatternMatchParameter(j, Parameters[j]->GetAtoms()[0]->ConvertToCompileTimeParam(program), program))
								{
									match = false;
									break;
								}
								Parameters[j]->AtomsArePatternMatchedLiteral = true;
							}
							else
							{
								match = false;
								break;
							}
						}
					}
					
					if(match)
					{
						Name = overloadname;
						MyType = func->GetReturnType(program);

						for(size_t j = 0; j < Parameters.size(); ++j)
						{
							if(func->IsParameterReference(func->GetParameterNames()[j]))
							{
								// TODO - bulletproof this a bit
								std::auto_ptr<ExpressionAtomIdentifier> atom(dynamic_cast<ExpressionAtomIdentifier*>(Parameters[j]->GetAtoms()[0]));
								Parameters[j]->GetAtoms()[0] = new ExpressionAtomIdentifierReference(atom->GetIdentifier(), atom->GetOriginalIdentifier());
								Parameters[j]->GetAtoms()[0]->TypeInference(program, activescope, newcontext, j, errors);
							}
						}

						break;
					}
				}
			}

			if(MyType == VM::EpochType_Error || MyType == VM::EpochType_Infer)
				errors.SemanticError("No matching overload");
		}
		else
		{
			OverloadMap::const_iterator overloadmapiter = program.Session.FunctionOverloadNames.find(Name);
			if(overloadmapiter != program.Session.FunctionOverloadNames.end())
			{
				const StringHandleSet& overloads = overloadmapiter->second;
				for(StringHandleSet::const_iterator overloaditer = overloads.begin(); overloaditer != overloads.end(); ++overloaditer)
				{
					const FunctionSignature& funcsig = program.Session.FunctionSignatures.find(*overloaditer)->second;
					if(funcsig.GetNumParameters() == Parameters.size())
					{
						bool match = true;
						for(size_t i = 0; i < Parameters.size(); ++i)
						{
							if(funcsig.GetParameter(i).Type != Parameters[i]->GetEpochType(program))
							{
								match = false;
								break;
							}
						}

						if(match)
						{
							Name = *overloaditer;
							MyType = funcsig.GetReturnType();

							for(size_t i = 0; i < Parameters.size(); ++i)
							{
								if(funcsig.GetParameter(i).IsReference)
								{
									// TODO - bulletproof this a bit
									std::auto_ptr<ExpressionAtomIdentifier> atom(dynamic_cast<ExpressionAtomIdentifier*>(Parameters[i]->GetAtoms()[0]));
									Parameters[i]->GetAtoms()[0] = new ExpressionAtomIdentifierReference(atom->GetIdentifier(), atom->GetOriginalIdentifier());
									Parameters[i]->GetAtoms()[0]->TypeInference(program, activescope, newcontext, i, errors);
								}
							}

							break;
						}
					}
				}

				if(MyType == VM::EpochType_Error || MyType == VM::EpochType_Infer)
					errors.SemanticError("No matching overload");
			}
			else
			{
				FunctionSignatureSet::const_iterator funciter = program.Session.FunctionSignatures.find(Name);
				if(funciter != program.Session.FunctionSignatures.end())
				{
					if(funciter->second.GetNumParameters() == Parameters.size())
					{
						bool match = true;
						for(size_t i = 0; i < Parameters.size(); ++i)
						{
							if(funciter->second.GetParameter(i).Type != Parameters[i]->GetEpochType(program))
							{
								match = false;
								break;
							}

							if(funciter->second.GetParameter(i).IsReference)
							{
								// TODO - bulletproof this a bit
								std::auto_ptr<ExpressionAtomIdentifier> atom(dynamic_cast<ExpressionAtomIdentifier*>(Parameters[i]->GetAtoms()[0]));
								Parameters[i]->GetAtoms()[0] = new ExpressionAtomIdentifierReference(atom->GetIdentifier(), atom->GetOriginalIdentifier());
								Parameters[i]->GetAtoms()[0]->TypeInference(program, activescope, newcontext, i, errors);
							}
						}

						if(match)
							MyType = funciter->second.GetReturnType();
						else
							errors.SemanticError("No matching overload");
					}
				}
				else
				{
					Function* func = program.GetFunctions().find(context.FunctionName)->second;
					if(func->HasParameter(Name))
						MyType = func->GetParameterSignatureType(Name, program);
					else
						errors.SemanticError("Undefined function");
				}
			}
		}
	}

	bool valid = (MyType != VM::EpochType_Infer && MyType != VM::EpochType_Error);
	return valid;
}


bool PreOpStatement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext&, CompileErrors&)
{
	VM::EpochTypeID operandtype = InferMemberAccessType(Operand, program, activescope);
	if(operandtype == VM::EpochType_Error)
		return false;

	StringHandleSet functionstocheck;

	const OverloadMap& overloads = *program.Session.InfoTable.Overloads;
	OverloadMap::const_iterator iter = overloads.find(OperatorName);
	if(iter == overloads.end())
		functionstocheck.insert(OperatorName);
	else
		functionstocheck = iter->second;

	if(functionstocheck.empty())
	{
		//
		// This is probably a failure of the library or whoever
		// wrote the preop implementation. The operator has been
		// registered in the grammar but not the overloads table,
		// so we can't figure out what parameters it takes.
		//
		throw InternalException("Preoperator defined in the grammar but no implementations could be located");
	}

	for(StringHandleSet::const_iterator iter = functionstocheck.begin(); iter != functionstocheck.end(); ++iter)
	{
		FunctionSignatureSet::const_iterator sigiter = program.Session.FunctionSignatures.find(*iter);
		if(sigiter == program.Session.FunctionSignatures.end())
		{
			//
			// This is another failure of the operator implementation.
			// The signature for the operator cannot be found.
			//
			throw InternalException("Preoperator defined but no signature provided");
		}

		if(sigiter->second.GetNumParameters() == 1 && sigiter->second.GetParameter(0).Type == operandtype)
		{
			OperatorName = *iter;
			MyType = sigiter->second.GetReturnType();
			break;
		}
	}

	return (MyType != VM::EpochType_Error);
}

bool PreOpStatement::Validate(const Program&) const
{
	return MyType != VM::EpochType_Error;
}


bool PostOpStatement::TypeInference(Program& program, CodeBlock& activescope, InferenceContext&, CompileErrors&)
{
	VM::EpochTypeID operandtype = InferMemberAccessType(Operand, program, activescope);
	if(operandtype == VM::EpochType_Error)
		return false;

	StringHandleSet functionstocheck;

	const OverloadMap& overloads = *program.Session.InfoTable.Overloads;
	OverloadMap::const_iterator iter = overloads.find(OperatorName);
	if(iter == overloads.end())
		functionstocheck.insert(OperatorName);
	else
		functionstocheck = iter->second;

	if(functionstocheck.empty())
	{
		//
		// This is probably a failure of the library or whoever
		// wrote the postop implementation. The operator has been
		// registered in the grammar but not the overloads table,
		// so we can't figure out what parameters it takes.
		//
		throw InternalException("Postoperator defined in the grammar but no implementations could be located");
	}

	for(StringHandleSet::const_iterator iter = functionstocheck.begin(); iter != functionstocheck.end(); ++iter)
	{
		FunctionSignatureSet::const_iterator sigiter = program.Session.FunctionSignatures.find(*iter);
		if(sigiter == program.Session.FunctionSignatures.end())
		{
			//
			// This is another failure of the operator implementation.
			// The signature for the operator cannot be found.
			//
			throw InternalException("Postoperator defined but no signature provided");
		}

		if(sigiter->second.GetNumParameters() == 1 && sigiter->second.GetParameter(0).Type == operandtype)
		{
			OperatorName = *iter;
			MyType = sigiter->second.GetReturnType();
			break;
		}
	}

	return (MyType != VM::EpochType_Error);
}

bool PostOpStatement::Validate(const Program&) const
{
	return MyType != VM::EpochType_Error;
}
