//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR classes for representing assignments
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Assignment.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Namespace.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"

#include "Compiler/Intermediate Representations/Semantic Validation/InferenceContext.h"

#include "Compiler/Exceptions.h"
#include "Compiler/CompileErrors.h"


using namespace IRSemantics;


//
// Construct and initialize an assignment IR node
//
Assignment::Assignment(const std::vector<StringHandle>& lhs, StringHandle operatorname, const AST::IdentifierT& originallhs)
	: LHS(lhs),
	  OperatorName(operatorname),
	  RHS(NULL),
	  LHSType(Metadata::EpochType_Error),
	  OriginalLHS(originallhs),
	  WantsTypeAnnotation(false),
	  HasAdditionalEffects(false)
{
}

//
// Destruct and clean up an assignment IR node
//
Assignment::~Assignment()
{
	delete RHS;
}

//
// Create a deep copy of an assignment IR node
//
Assignment* Assignment::Clone() const
{
	Assignment* clone = new Assignment(LHS, OperatorName, OriginalLHS);
	clone->RHS = RHS->Clone();
	clone->LHSType = LHSType;
	return clone;
}


//
// Recursively traverse a chain of assignments and set
// the farthest-right side to the given new chain
//
void Assignment::SetRHSRecursive(AssignmentChain* rhs)
{
	if(RHS)
	{
		if(!RHS->CanChainToAssignment())
		{
			//
			// This should basically never happen unless the parser is
			// generating gibberish in the AST.
			//
			// Basically this is designed to ensure that everything in
			// the chain is an l-value, and the far right hand side is
			// an r-value. The semantics of this may need to change if
			// we wish to permit arbitrary expressions to be l-values.
			//
			// For clarity, assignments must be of the form:
			//  l-value [ = l-value ...] = r-value
			//
			// Where an l-value is either a primitive variable or a
			// (possibly deeply nested) structure member, and an r-value
			// is an arbitrary expression of compatible type to the
			// chain of l-values.
			//
			throw InternalException("Far-right of assignment or assignment chain cannot participate in further chaining");
		}

		RHS->SetRHSRecursive(rhs);
	}
	else
	{
		delete RHS;
		RHS = rhs;
	}
}

//
// Type-validate an assignment
//
bool Assignment::Validate(const Namespace& curnamespace) const
{
	bool valid = (LHSType != Metadata::EpochType_Error);
	return valid && RHS->Validate(curnamespace);
}

//
// Perform type inference actions on an assignment
//
// Includes operator overload resolution for op-assignment
// forms such as "a += b".
//
bool Assignment::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	errors.SetContext(OriginalLHS);
	LHSType = InferMemberAccessType(LHS, curnamespace, activescope, errors);
	if(LHSType == Metadata::EpochType_Error)
		return false;

	if(!RHS->TypeInference(curnamespace, activescope, context, errors))
		return false;

	if(curnamespace.Functions.HasOverloads(OperatorName))
	{
		const StringHandleSet& overloads = curnamespace.Functions.GetOverloadNames(OperatorName);
		for(StringHandleSet::const_iterator overloaditer = overloads.begin(); overloaditer != overloads.end(); ++overloaditer)
		{
			const FunctionSignature& overloadsig = curnamespace.Functions.GetSignature(*overloaditer);
			if(overloadsig.GetNumParameters() == 2)
			{
				if(overloadsig.GetParameter(0).Type == Metadata::MakeNonReferenceType(LHSType) && overloadsig.GetParameter(1).Type == Metadata::MakeNonReferenceType(RHS->GetEpochType(curnamespace)))
				{
					OperatorName = *overloaditer;
					break;
				}
			}
		}
	}

	Metadata::EpochTypeID RHSType = RHS->GetEpochType(curnamespace);
	if(Metadata::MakeNonReferenceType(LHSType) != Metadata::MakeNonReferenceType(RHSType))
	{
		if(Metadata::GetTypeFamily(LHSType) == Metadata::EpochTypeFamily_Unit && curnamespace.Types.Aliases.GetStrongRepresentation(LHSType) == RHSType)
		{
			// OK
		}
		else if(Metadata::GetTypeFamily(LHSType) == Metadata::EpochTypeFamily_SumType && curnamespace.Types.SumTypes.IsBaseType(LHSType, Metadata::MakeNonReferenceType(RHSType)))
		{
			// OK
			WantsTypeAnnotation = true;
		}
		else
		{
			errors.SetContext(OriginalLHS);
			errors.SemanticError("Left-hand side of assignment differs in type from right-hand side");
			LHSType = Metadata::EpochType_Error;
		}
	}

	return true;
}


//
// Construct and initialize the RHS of an assignment
// which is terminated with an expression
//
AssignmentChainExpression::AssignmentChainExpression(Expression* expression)
	: MyExpression(expression)
{
}

//
// Destruct and clean up a terminal RHS
//
AssignmentChainExpression::~AssignmentChainExpression()
{
	delete MyExpression;
}

//
// Retrieve the type of an assignment RHS which
// is a terminal expression
//
Metadata::EpochTypeID AssignmentChainExpression::GetEpochType(const Namespace& curnamespace) const
{
	return MyExpression->GetEpochType(curnamespace);
}

//
// Perform type inference actions on a terminal assignment RHS
//
bool AssignmentChainExpression::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	InferenceContext newcontext(0, InferenceContext::CONTEXT_ASSIGNMENT);
	newcontext.FunctionName = context.FunctionName;
	return MyExpression->TypeInference(curnamespace, activescope, newcontext, 0, 1, errors);
}

//
// Validate the expression appearing on the RHS of an assignment
//
bool AssignmentChainExpression::Validate(const Namespace& curnamespace) const
{
	return MyExpression->Validate(curnamespace);
}



//
// Construct and initialize an assignment which chains to another assignment
//
// This handles the "b" fragment of the assignment "a = b = 42"
//
AssignmentChainAssignment::AssignmentChainAssignment(Assignment* assignment)
	: MyAssignment(assignment)
{
}

//
// Destruct and clean up a chained assignment
//
AssignmentChainAssignment::~AssignmentChainAssignment()
{
	delete MyAssignment;
}

//
// Recursively traverse a chained assignment until
// the current terminal LHS is found, and attach it
// to the given RHS, which may not be terminal.
//
void AssignmentChainAssignment::SetRHSRecursive(AssignmentChain* rhs)
{
	MyAssignment->SetRHSRecursive(rhs);
}

//
// Retrieve the type of the RHS of a chained assignment
//
Metadata::EpochTypeID AssignmentChainAssignment::GetEpochType(const Namespace& curnamespace) const
{
	return MyAssignment->GetRHS()->GetEpochType(curnamespace);
}

//
// Pass along the request to perform type inference
// to the next assignment in a chain
//
bool AssignmentChainAssignment::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	return MyAssignment->TypeInference(curnamespace, activescope, context, errors);
}

//
// Pass along the request to perform type validation
// to the next assignment in a chain
//
bool AssignmentChainAssignment::Validate(const Namespace& curnamespace) const
{
	return MyAssignment->Validate(curnamespace);
}


//
// Deep copy a terminal RHS of an assignment
//
AssignmentChain* AssignmentChainExpression::Clone() const
{
	return new AssignmentChainExpression(MyExpression->Clone());
}

//
// Deep copy an intermediate assignment in a chain
//
AssignmentChain* AssignmentChainAssignment::Clone() const
{
	return new AssignmentChainAssignment(MyAssignment->Clone());
}
