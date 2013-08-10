//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST traverser declaration for the compilation pass which
// performs semantic validation and type inference on the
// input program code. This traverser converts the AST into
// an intermediate representation (IR) which is then used
// by the compiler to perform semantic checks etc.
//

#include "pch.h"

#include "Compiler/Passes/SemanticValidation.h"

#include "Compiler/Abstract Syntax Tree/Literals.h"
#include "Compiler/Abstract Syntax Tree/Parenthetical.h"
#include "Compiler/Abstract Syntax Tree/Expression.h"
#include "Compiler/Abstract Syntax Tree/Statement.h"
#include "Compiler/Abstract Syntax Tree/Entities.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Structure.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Assignment.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Statement.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Entity.h"

#include "Compiler/Session.h"

#include "Utility/Strings.h"

#include "User Interface/Output.h"

#include <algorithm>


//
// Internal helpers
//
namespace
{

	struct StringPooler
	{
		StringPooler(IRSemantics::Program& program, std::vector<StringHandle>& vec)
			: Program(program),
			  Container(vec)
		{ }

		void operator () (const AST::IdentifierT& identifier)
		{
			StringHandle handle = Program.AddString(std::wstring(identifier.begin(), identifier.end()));
			Container.push_back(handle);
		}

	// Assignment prohibited
	private:
		StringPooler& operator = (const StringPooler& rhs);

	private:
		IRSemantics::Program& Program;
		std::vector<StringHandle>& Container;
	};

}


//
// Validate semantics for a program
//
IRSemantics::Program* CompilerPasses::ValidateSemantics(AST::Program& program, const std::wstring::const_iterator& sourcebegin, const std::wstring::const_iterator& sourceend, StringPoolManager& strings, CompileSession& session)
{
	// Construct the semantic analysis pass
	ASTTraverse::CompilePassSemantics pass(strings, session, sourcebegin, sourceend);
	
	// Traverse the AST and convert it into the semantic IR
	ASTTraverse::DoTraversal(pass, program);

	if(pass.Errors.HasErrors())
	{
		pass.Errors.DumpErrors();
		return NULL;
	}

	// Perform compile-time code execution, e.g. constructors for populating lexical scopes
	if(!pass.CompileTimeCodeExecution())
	{
		UI::OutputStream output;
		output << UI::lightred << "Failed compiled time code execution!" << UI::white << std::endl;
		pass.Errors.DumpErrors();
		return NULL;
	}

	// Perform type inference and decorate the IR with type information
	if(!pass.TypeInference())
	{
		UI::OutputStream output;
		output << UI::lightred << "Failed type inference!" << UI::white << std::endl;
		pass.Errors.DumpErrors();
		return NULL;
	}

	// Perform type validation
	if(!pass.Validate())
	{
		UI::OutputStream output;
		output << UI::lightred << "Failed type validation!" << UI::white << std::endl;
		pass.Errors.DumpErrors();
		return NULL;
	}

	// Success!
	return pass.DetachProgram();
}


using namespace ASTTraverse;


//
// Destruct and clean up a program semantic verification pass
//
CompilePassSemantics::~CompilePassSemantics()
{
	for(std::vector<IRSemantics::Structure*>::iterator iter = CurrentStructures.begin(); iter != CurrentStructures.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::Function*>::iterator iter = CurrentFunctions.begin(); iter != CurrentFunctions.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::Expression*>::iterator iter = CurrentExpressions.begin(); iter != CurrentExpressions.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::Assignment*>::iterator iter = CurrentAssignments.begin(); iter != CurrentAssignments.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::Statement*>::iterator iter = CurrentStatements.begin(); iter != CurrentStatements.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::CodeBlock*>::iterator iter = CurrentCodeBlocks.begin(); iter != CurrentCodeBlocks.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::Entity*>::iterator iter = CurrentEntities.begin(); iter != CurrentEntities.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::Entity*>::iterator iter = CurrentChainedEntities.begin(); iter != CurrentChainedEntities.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::Entity*>::iterator iter = CurrentPostfixEntities.begin(); iter != CurrentPostfixEntities.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::FunctionParamFuncRef*>::iterator iter = CurrentFunctionSignatures.begin(); iter != CurrentFunctionSignatures.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::FunctionTag*>::iterator iter = CurrentFunctionTags.begin(); iter != CurrentFunctionTags.end(); ++iter)
		delete *iter;

	for(std::vector<CompileTimeParameterVector*>::iterator iter = CurrentTemplateArgs.begin(); iter != CurrentTemplateArgs.end(); ++iter)
		delete *iter;

	delete CurrentProgram;
}

//
// Perform compile-time code execution on a program
//
bool CompilePassSemantics::CompileTimeCodeExecution()
{
	if(!CurrentProgram)
		return false;

	bool result = CurrentProgram->CompileTimeCodeExecution(Errors);
	if(!result || Errors.HasErrors())
	{
		Errors.DumpErrors();
		return false;
	}

	return true;
}

//
// Perform type inference/type decoration on a program
//
bool CompilePassSemantics::TypeInference()
{
	if(!CurrentProgram)
		return false;
	
	bool result = CurrentProgram->TypeInference(Errors);
	if(!result || Errors.HasErrors())
	{
		Errors.DumpErrors();
		return false;
	}

	return true;
}

//
// Validate a program
//
bool CompilePassSemantics::Validate()
{
	if(!CurrentProgram)
		return false;

	if(!CurrentProgram->Validate(Errors) || Errors.HasErrors())
	{
		Errors.DumpErrors();
		return false;
	}

	return true;
}

//
// Detach and return the converted program IR
//
IRSemantics::Program* CompilePassSemantics::DetachProgram()
{
	IRSemantics::Program* p = NULL;
	std::swap(p, CurrentProgram);
	return p;
}


//
// Begin traversing a node with no defined type
//
void CompilePassSemantics::EntryHelper::operator () (AST::Undefined&)
{
	if(self->StateStack.empty() || !self->InFunctionReturn)
	{
		if(!self->StateStack.empty())
		{
			switch(self->StateStack.top())
			{
			case STATE_STRUCTURE:
			case STATE_TEMPLATE_ARGS:
			case STATE_FUNCTION:
			case STATE_STRUCTURE_FUNCTION_RETURN:
			case STATE_FUNCTION_SIGNATURE_RETURN:
			case STATE_FUNCTION_PARAM:
			case STATE_SUM_TYPE:
				return;
			}
		}

		//
		// This is a failure of the parser to properly
		// capture incorrect programs prior to being
		// submitted for semantic validation.
		//
		// Undefined nodes are permitted in several situations:
		//  1. Empty programs
		//  2. Void function return expressions
		//  3. Empty function tag specifiers
		//  4. Omitted function code blocks
		//  5. Omitted "ref" tag on function parameters
		//  6. Omitted template parameters/arguments
		//
		// Any other presence of an undefined node represents
		// a mistake in the parser. Ensure that parser errors
		// are detected correctly and that programs which are
		// not fully parsed can not be submitted for semantic
		// validation passes.
		//
		throw InternalException("Undefined AST node in unexpected context");
	}
}


//
// Begin traversing a program node, which represents the root of the AST
//
void CompilePassSemantics::EntryHelper::operator () (AST::Program&)
{
	if(self->CurrentProgram)
	{
		//
		// The parser has generated a program which contains
		// either a cyclic reference to itself or to another
		// AST. This is undesirable behavior; the parser can
		// produce multiple AST fragments for the purpose of
		// separate compilation but they must be merged into
		// a single program-wide AST prior to the performing
		// semantic analysis, or submitted as fully separate
		// tree structures within a compilation session.
		//
		throw InternalException("Re-entrant AST detected");
	}

	self->StateStack.push(CompilePassSemantics::STATE_PROGRAM);
	self->CurrentProgram = new IRSemantics::Program(self->Strings, self->Session);
	self->CurrentNamespace = &self->CurrentProgram->GlobalNamespace;
}

//
// Finish traversing a program node
//
void CompilePassSemantics::ExitHelper::operator () (AST::Program&)
{
	self->StateStack.pop();
}


//
// Begin traversing a structure definition node
//
void CompilePassSemantics::EntryHelper::operator () (AST::Structure&)
{
	self->StateStack.push(CompilePassSemantics::STATE_STRUCTURE);
	self->CurrentStructures.push_back(new IRSemantics::Structure);
}

//
// Finish traversing a structure definition node
//
void CompilePassSemantics::ExitHelper::operator () (AST::Structure& structure)
{
	self->StateStack.pop();
	if(self->CurrentStructures.size() == 1)
	{
		std::auto_ptr<IRSemantics::Structure> irstruct(self->CurrentStructures.back());
		self->CurrentStructures.pop_back();
		
		StringHandle name = self->CurrentProgram->AddString(std::wstring(structure.Identifier.begin(), structure.Identifier.end()));
		self->ErrorContext = &structure.Identifier;
		self->CurrentNamespace->Types.Structures.Add(name, irstruct.release(), self->Errors);
		self->ErrorContext = NULL;
	}
	else
	{
		//
		// This is plain and simple a missing language feature.
		//
		// Nested structure definitions have been parsed and sent
		// to the AST for semantic validation, but the above code
		// is not configured to handle this. Implement the code.
		//
		throw InternalException("Support for nested structure definitions is not implemented.");
	}
}


//
// Traverse a node defining a member variable in a structure
//
void CompilePassSemantics::EntryHelper::operator () (AST::StructureMemberVariable& variable)
{
	if(self->CurrentStructures.empty())
	{
		//
		// This is a failure of the AST traversal.
		//
		// A structure member variable definition node has
		// been submitted for semantic validation, but not
		// in the context of a structure definition.
		//
		// Examine the AST generation and traversal logic.
		//
		throw InternalException("Attempted to traverse a structure member AST node in an invalid context");
	}

	StringHandle name = self->CurrentProgram->AddString(std::wstring(variable.Name.begin(), variable.Name.end()));
	StringHandle type = self->CurrentProgram->AddString(std::wstring(variable.Type.begin(), variable.Type.end()));

	std::auto_ptr<IRSemantics::StructureMemberVariable> member(new IRSemantics::StructureMemberVariable(type, variable.Type));
	self->ErrorContext = &variable.Name;
	self->CurrentStructures.back()->AddMember(name, member.release(), self->Errors);
	self->ErrorContext = NULL;
	self->LastStructureMemberName = name;
}

//
// Traverse a node defining a member function reference in a structure
//
void CompilePassSemantics::EntryHelper::operator () (AST::StructureMemberFunctionRef& func)
{
	if(self->CurrentStructures.empty())
	{
		//
		// This is a failure of the AST traversal.
		//
		// A structure member variable definition node has
		// been submitted for semantic validation, but not
		// in the context of a structure definition.
		//
		// Specifically, the member variable is a function
		// reference with the associated signature.
		//
		// Examine the AST generation and traversal logic.
		//
		throw InternalException("Attempted to traverse a structure member AST node in an invalid context");
	}

	self->StateStack.push(CompilePassSemantics::STATE_STRUCTURE_FUNCTION);
	self->CurrentStructureFunctions.push_back(new IRSemantics::StructureMemberFunctionReference(func.Name));
}

void CompilePassSemantics::ExitHelper::operator () (AST::StructureMemberFunctionRef& funcref)
{
	StringHandle name = self->CurrentProgram->AddString(std::wstring(funcref.Name.begin(), funcref.Name.end()));

	self->ErrorContext = &funcref.Name;
	self->CurrentStructures.back()->AddMember(name, self->CurrentStructureFunctions.back(), self->Errors);
	self->ErrorContext = NULL;
	self->CurrentStructureFunctions.pop_back();
	self->StateStack.pop();
}


//
// Begin traversing a node that defines a function
//
void CompilePassSemantics::EntryHelper::operator () (AST::Function&)
{
	self->StateStack.push(CompilePassSemantics::STATE_FUNCTION);
	self->CurrentFunctions.push_back(new IRSemantics::Function);
}

//
// Finish traversing a node that defines a function
//
void CompilePassSemantics::ExitHelper::operator () (AST::Function& function)
{
	self->StateStack.pop();

	if(!self->CurrentFunctions.back()->GetCode())
	{
		std::auto_ptr<ScopeDescription> lexicalscope(new ScopeDescription(self->CurrentNamespace->GetGlobalScope()));
		self->CurrentFunctions.back()->SetCode(new IRSemantics::CodeBlock(lexicalscope.release()));
	}

	if(!self->CurrentFunctions.back()->IsTemplate())
		self->CurrentFunctions.back()->PopulateScope(*self->CurrentNamespace, self->Errors);

	if(self->CurrentFunctions.size() == 1)
	{
		std::wstring rawnamestr = std::wstring(function.Name.begin(), function.Name.end());
		StringHandle rawname = self->CurrentProgram->AddString(rawnamestr);
		self->ErrorContext = &function.Name;
		StringHandle name = self->CurrentNamespace->Functions.CreateOverload(rawnamestr);
		self->CurrentNamespace->Functions.Add(name, rawname, self->CurrentFunctions.back());
		self->CurrentFunctions.back()->SetName(name);
		self->CurrentFunctions.pop_back();
		self->ErrorContext = NULL;
	}
	else
	{
		//
		// This is just a missing language feature.
		//
		// Nested/inner functions should be supported eventually,
		// and in this case the parser has generated an AST which
		// contains such a function, but the above code isn't set
		// up to handle it.
		//
		throw InternalException("A nested (inner) function was produced by the parser, but support is not implemented");
	}
}


//
// Begin traversing a node that defines a function parameter
//
void CompilePassSemantics::EntryHelper::operator () (AST::FunctionParameter&)
{
	self->StateStack.push(STATE_FUNCTION_PARAM);
}

//
// Finish traversing a node that defines a function parameter
//
void CompilePassSemantics::ExitHelper::operator () (AST::FunctionParameter&)
{
	self->StateStack.pop();
}


//
// Traverse a node that corresponds to a named function parameter
//
// We can safely treat these as leaf nodes, hence the direct access of the
// node's properties to retrieve its contents.
//
void CompilePassSemantics::EntryHelper::operator () (AST::NamedFunctionParameter&)
{
	self->IsParamRef = false;
	self->CurrentTemplateArgsScratch.clear();
}

void CompilePassSemantics::ExitHelper::operator () (AST::NamedFunctionParameter& param)
{
	if(self->CurrentFunctions.empty())
	{
		//
		// This is a failure of the AST traversal.
		//
		// A function parameter definition node has been
		// traversed outside the context of a function
		// definition.
		//
		// Examine the AST generation and traversal logic.
		//
		throw InternalException("Attempted to traverse a function parameter definition AST node in an invalid context");
	}


	StringHandle name = self->CurrentProgram->AddString(std::wstring(param.Name.begin(), param.Name.end()));
	StringHandle type = self->CurrentProgram->AddString(std::wstring(param.Type.begin(), param.Type.end()));

	self->ErrorContext = &param.Name;
	std::auto_ptr<IRSemantics::FunctionParamNamed> irparam(new IRSemantics::FunctionParamNamed(type, self->CurrentTemplateArgsScratch, self->IsParamRef));
	self->CurrentFunctions.back()->AddParameter(name, irparam.release(), false, self->Errors);
	self->ErrorContext = NULL;
}


void CompilePassSemantics::EntryHelper::operator () (AST::FunctionTag&)
{
	self->StateStack.push(CompilePassSemantics::STATE_FUNCTION_TAG);
	self->CurrentFunctionTags.push_back(new IRSemantics::FunctionTag);
}

void CompilePassSemantics::ExitHelper::operator () (AST::FunctionTag&)
{
	self->CurrentFunctions.back()->AddTag(*self->CurrentFunctionTags.back());

	self->StateStack.pop();
	self->StateStack.pop();
	delete self->CurrentFunctionTags.back();
	self->CurrentFunctionTags.pop_back();
}


//
// Begin traversing a node that corresponds to an expression
//
void CompilePassSemantics::EntryHelper::operator () (AST::Expression&)
{
	self->StateStack.push(CompilePassSemantics::STATE_EXPRESSION);
	self->CurrentExpressions.push_back(new IRSemantics::Expression);
}

//
// Finish traversing an expression node
//
void CompilePassSemantics::ExitHelper::operator () (AST::Expression&)
{
	self->StateStack.pop();

	switch(self->StateStack.top())
	{
	default:
	case CompilePassSemantics::STATE_UNKNOWN:
		//
		// This is most likely a failure in the AST traversal
		// implementation; either we have explicitly stated
		// that the state of traversal is unknown (which should
		// never occur) or we are in a state not specifically
		// handled by another case.
		//
		// Examine the current state and why the AST traverser
		// is reporting that state. If necessary, implement
		// support for expression nodes appearing in that state.
		// Otherwise, fix the AST so it doesn't traverse over
		// expression nodes in that particular way.
		//
		throw InternalException("Parse state is explicitly unknown or otherwise unrecognized");

	case CompilePassSemantics::STATE_EXPRESSION_COMPONENT:
	case CompilePassSemantics::STATE_EXPRESSION_FRAGMENT:
		{
			std::auto_ptr<IRSemantics::Expression> expr(self->CurrentExpressions.back());
			self->CurrentExpressions.pop_back();

			std::auto_ptr<IRSemantics::Parenthetical> parenthetical(new IRSemantics::ParentheticalExpression(expr.release()));
			std::auto_ptr<IRSemantics::ExpressionAtomParenthetical> atom(new IRSemantics::ExpressionAtomParenthetical(parenthetical.release()));
			self->CurrentExpressions.back()->AddAtom(atom.release());
		}
		break;

	case CompilePassSemantics::STATE_STATEMENT:
		if(self->CurrentStatements.empty())
		{
			//
			// For some reason the AST traversal logic thinks
			// that we should be inside a statement, but for
			// some other reason, no statement has been placed
			// on the active statements list.
			//
			// This is probably an egregious bug in the parser.
			//
			throw InternalException("Parser thinks it is traversing a statement, but no statements are actively being parsed");
		}

		self->CurrentStatements.back()->AddParameter(self->CurrentExpressions.back());
		self->CurrentExpressions.pop_back();
		break;

	case CompilePassSemantics::STATE_ASSIGNMENT:
		if(self->CurrentAssignments.empty())
		{
			//
			// For some reason the AST traversal logic thinks
			// that we should be inside an assignment, but for
			// some other reason, no assignment has been placed
			// on the active assignments list.
			//
			// This is probably an egregious bug in the parser.
			//
			throw InternalException("Parser thinks it is traversing an assignment, but no assignments are actively being parsed");
		}

		self->CurrentAssignments.back()->SetRHSRecursive(new IRSemantics::AssignmentChainExpression(self->CurrentExpressions.back()));
		self->CurrentExpressions.pop_back();
		break;

	case CompilePassSemantics::STATE_ENTITY:
		self->CurrentEntities.back()->AddParameter(self->CurrentExpressions.back());
		self->CurrentExpressions.pop_back();
		break;

	case CompilePassSemantics::STATE_CHAINED_ENTITY:
		self->CurrentChainedEntities.back()->AddParameter(self->CurrentExpressions.back());
		self->CurrentExpressions.pop_back();
		break;

	case CompilePassSemantics::STATE_POSTFIX_ENTITY:
		self->CurrentPostfixEntities.back()->AddParameter(self->CurrentExpressions.back());
		self->CurrentExpressions.pop_back();
		break;

	case CompilePassSemantics::STATE_FUNCTION_RETURN:
		// Nothing needs to be done; leave the expression on the state stack
		// When the FunctionReturnExpression marker is exited it will handle the expression correctly
		break;

	case CompilePassSemantics::STATE_FUNCTION_PARAM:
		if(self->CurrentFunctions.empty())
		{
			//
			// For some reason the AST traversal logic thinks
			// that we should be inside a function definition,
			// but for some other reason, no function has been
			// placed on the active functions list.
			//
			// This is probably an egregious bug in the parser.
			//
			throw InternalException("Parser thinks it is traversing a function definition, but no functions are actively being parsed");
		}
		else
		{
			StringHandle paramname = self->CurrentNamespace->AllocateAnonymousParamName();

			std::auto_ptr<IRSemantics::FunctionParamExpression> irparam(new IRSemantics::FunctionParamExpression(self->CurrentExpressions.back()));
			self->CurrentExpressions.pop_back();

			self->CurrentFunctions.back()->AddParameter(paramname, irparam.release(), false, self->Errors);
		}
		break;
	}
}

//
// Begin traversing an expression component node (see AST definitions for details)
//
void CompilePassSemantics::EntryHelper::operator () (AST::ExpressionComponent&)
{
	self->StateStack.push(CompilePassSemantics::STATE_EXPRESSION_COMPONENT);
}

//
// Finish traversing an expression component node
//
void CompilePassSemantics::ExitHelper::operator () (AST::ExpressionComponent&)
{
	self->StateStack.pop();
}

//
// Begin traversing an expression fragment node (see AST definitions for details)
//
void CompilePassSemantics::EntryHelper::operator () (AST::ExpressionFragment& exprfragment)
{
	self->StateStack.push(CompilePassSemantics::STATE_EXPRESSION_FRAGMENT);

	std::wstring token(exprfragment.Operator.begin(), exprfragment.Operator.end());
	StringHandle opname = self->CurrentProgram->AddString(token);
	self->CurrentExpressions.back()->AddAtom(new IRSemantics::ExpressionAtomOperator(opname, token == L".", 0));
}

//
// Finish traversing an expression fragment node
//
void CompilePassSemantics::ExitHelper::operator () (AST::ExpressionFragment&)
{
	self->StateStack.pop();
}


//
// Begin traversing a statement node
//
void CompilePassSemantics::EntryHelper::operator () (AST::Statement& statement)
{
	StringHandle namehandle = self->CurrentProgram->AddString(std::wstring(statement.Identifier.begin(), statement.Identifier.end()));

	self->StateStack.push(CompilePassSemantics::STATE_STATEMENT);
	self->CurrentStatements.push_back(new IRSemantics::Statement(namehandle, statement.Identifier));
}

//
// Finish traversing a statement node
//
void CompilePassSemantics::ExitHelper::operator () (AST::Statement&)
{
	self->StateStack.pop();

	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_EXPRESSION_COMPONENT:
	case CompilePassSemantics::STATE_EXPRESSION_FRAGMENT:
		self->CurrentExpressions.back()->AddAtom(new IRSemantics::ExpressionAtomStatement(self->CurrentStatements.back()));
		self->CurrentStatements.pop_back();
		break;

	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockStatementEntry(self->CurrentStatements.back()));
		self->CurrentStatements.pop_back();
		break;

	default:
		//
		// This is probably a missing language feature.
		//
		// An expression AST node has been traversed but its
		// surrounding context was not explicitly handled by
		// one of the above cases. Ensure the AST generation
		// is correct and that the context is supported.
		//
		throw InternalException("Expression occurs in an unrecognized context");
	}
}


//
// Begin traversing a node corresponding to a pre-operation statement
//
void CompilePassSemantics::EntryHelper::operator () (AST::PreOperatorStatement&)
{
	self->StateStack.push(CompilePassSemantics::STATE_PREOP_STATEMENT);
}

//
// Finish traversing a node corresponding to a pre-operation statement
//
void CompilePassSemantics::ExitHelper::operator () (AST::PreOperatorStatement& statement)
{
	self->StateStack.pop();

	StringHandle operatorname = self->CurrentProgram->AddString(std::wstring(statement.Operator.begin(), statement.Operator.end()));

	std::vector<StringHandle> operand;
	std::for_each(statement.Operand.begin(), statement.Operand.end(), StringPooler(*self->CurrentProgram, operand));

	std::auto_ptr<IRSemantics::PreOpStatement> irstatement(new IRSemantics::PreOpStatement(operatorname, operand, *statement.Operand.begin()));

	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_EXPRESSION_COMPONENT:
		{
			std::auto_ptr<IRSemantics::ParentheticalPreOp> irparenthetical(new IRSemantics::ParentheticalPreOp(irstatement.release()));
			self->CurrentExpressions.back()->AddAtom(new IRSemantics::ExpressionAtomParenthetical(irparenthetical.release()));
		}
		break;

	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockPreOpStatementEntry(irstatement.release()));
		break;

	default:
		//
		// This is probably a missing feature.
		//
		// A pre-operator statement (i.e. of the form ++x) has appeared
		// in the AST in some context which is not explicitly handled by
		// any of the above cases.
		//
		// Implement support for preop statements in the appropriate
		// context, or fix the AST to not generate these nodes in that
		// context to begin with.
		//
		throw InternalException("Pre-operator statement appears in an unrecognized AST location");
	}
}


//
// Begin traversing a node corresponding to a post-operation statement
//
void CompilePassSemantics::EntryHelper::operator () (AST::PostOperatorStatement&)
{
	self->StateStack.push(CompilePassSemantics::STATE_POSTOP_STATEMENT);
}

//
// Finish traversing a node corresponding to a post-operation statement
//
void CompilePassSemantics::ExitHelper::operator () (AST::PostOperatorStatement& statement)
{
	self->StateStack.pop();

	StringHandle operatorname = self->CurrentProgram->AddString(std::wstring(statement.Operator.begin(), statement.Operator.end()));

	std::vector<StringHandle> operand;
	std::for_each(statement.Operand.begin(), statement.Operand.end(), StringPooler(*self->CurrentProgram, operand));

	std::auto_ptr<IRSemantics::PostOpStatement> irstatement(new IRSemantics::PostOpStatement(operand, operatorname, *statement.Operand.begin()));

	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_EXPRESSION_COMPONENT:
		{
			std::auto_ptr<IRSemantics::ParentheticalPostOp> irparenthetical(new IRSemantics::ParentheticalPostOp(irstatement.release()));
			self->CurrentExpressions.back()->AddAtom(new IRSemantics::ExpressionAtomParenthetical(irparenthetical.release()));
		}
		break;

	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockPostOpStatementEntry(irstatement.release()));
		break;

	default:
		//
		// This is probably a missing feature.
		//
		// A post-operator statement (i.e. of the form x++) has appeared
		// in the AST in some context which is not explicitly handled by
		// any of the above cases.
		//
		// Implement support for postop statements in the appropriate
		// context, or fix the AST to not generate these nodes in that
		// context to begin with.
		//
		throw InternalException("Post-operator statement appears in an unrecognized AST location");
	}
}


//
// Begin traversing a node containing a code block
//
void CompilePassSemantics::EntryHelper::operator () (AST::CodeBlock&)
{
	bool owned = true;
	ScopeDescription* lexicalscope = NULL;

	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_PROGRAM:
		lexicalscope = self->CurrentNamespace->GetGlobalScope();
		owned = false;
		break;

	case CompilePassSemantics::STATE_FUNCTION:
		lexicalscope = new ScopeDescription(self->CurrentNamespace->GetGlobalScope());
		break;

	default:
		lexicalscope = new ScopeDescription(self->CurrentCodeBlocks.back()->GetScope());
		break;
	}

	self->CurrentCodeBlocks.push_back(new IRSemantics::CodeBlock(lexicalscope, owned));
	self->StateStack.push(CompilePassSemantics::STATE_CODE_BLOCK);
}

//
// Finish traversing a code block node
//
void CompilePassSemantics::ExitHelper::operator () (AST::CodeBlock&)
{
	self->CurrentNamespace->AllocateLexicalScopeName(self->CurrentCodeBlocks.back()->GetScope());

	self->StateStack.pop();
	
	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockInnerBlockEntry(self->CurrentCodeBlocks.back()));
		break;

	case CompilePassSemantics::STATE_FUNCTION:
		self->CurrentFunctions.back()->SetCode(self->CurrentCodeBlocks.back());
		break;

	case CompilePassSemantics::STATE_PROGRAM:
		self->CurrentNamespace->AddGlobalCodeBlock(self->CurrentCodeBlocks.back());
		break;

	case CompilePassSemantics::STATE_ENTITY:
		self->CurrentEntities.back()->SetCode(self->CurrentCodeBlocks.back());
		break;

	case CompilePassSemantics::STATE_CHAINED_ENTITY:
		self->CurrentChainedEntities.back()->SetCode(self->CurrentCodeBlocks.back());
		break;

	case CompilePassSemantics::STATE_POSTFIX_ENTITY:
		self->CurrentPostfixEntities.back()->SetCode(self->CurrentCodeBlocks.back());
		break;

	default:
		//
		// This is probably a missing feature.
		//
		// A code block has appeared in the AST in some context which
		// is not explicitly handled by any of the above cases.
		//
		// Implement support for code blocks in the appropriate context
		// or fix the AST to not generate these nodes in that context
		// to begin with.
		//
		throw InternalException("Code block appears in an unrecognized AST location");
	}

	self->CurrentCodeBlocks.pop_back();
}


//
// Begin traversing a node corresponding to an assignment
//
void CompilePassSemantics::EntryHelper::operator () (AST::Assignment& assignment)
{
	CompilePassSemantics::States state = self->StateStack.top();

	self->StateStack.push(CompilePassSemantics::STATE_ASSIGNMENT);

	std::wstring opnamestr(assignment.Operator.begin(), assignment.Operator.end());
	StringHandle opname = self->CurrentProgram->AddString(opnamestr);

	bool hasadditionaleffects = (opnamestr != L"=");

	std::vector<StringHandle> lhs;
	std::for_each(assignment.LHS.begin(), assignment.LHS.end(), StringPooler(*self->CurrentProgram, lhs));

	switch(state)
	{
	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentAssignments.push_back(new IRSemantics::Assignment(lhs, opname, *assignment.LHS.begin()));
		self->CurrentAssignments.back()->HasAdditionalEffects = hasadditionaleffects;
		break;

	case CompilePassSemantics::STATE_ASSIGNMENT:
		{
			std::auto_ptr<IRSemantics::Assignment> irassignment(new IRSemantics::Assignment(lhs, opname, *assignment.LHS.begin()));
			irassignment->HasAdditionalEffects = hasadditionaleffects;
			std::auto_ptr<IRSemantics::AssignmentChainAssignment> irassignmentchain(new IRSemantics::AssignmentChainAssignment(irassignment.release()));
			self->CurrentAssignments.back()->SetRHSRecursive(irassignmentchain.release());
		}
		break;

	default:
		//
		// This is probably a missing language feature.
		//
		// An assignment AST node has been traversed but its
		// surrounding context was not explicitly handled by
		// one of the above cases. Ensure the AST generation
		// is correct and that the context is supported.
		//
		throw InternalException("Assignment occurs in unrecognized context");
	}
}

//
// Finish traversing a node corresponding to an assignment
//
void CompilePassSemantics::ExitHelper::operator () (AST::Assignment&)
{
	self->StateStack.pop();
	
	if(self->StateStack.top() == CompilePassSemantics::STATE_CODE_BLOCK)
	{
		self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockAssignmentEntry(self->CurrentAssignments.back()));
		self->CurrentAssignments.pop_back();
	}
	else if(self->StateStack.top() == CompilePassSemantics::STATE_ASSIGNMENT)
	{
		// Nothing to do
	}
	else
	{
		//
		// This is probably a missing language feature.
		//
		// An assignment AST node has been traversed but its
		// surrounding context was not explicitly handled by
		// one of the above cases. Ensure the AST generation
		// is correct and that the context is supported.
		//
		throw InternalException("Assignment occurs in unrecognized context");
	}
}

//
// Begin traversing a node corresponding to an initialization
//
void CompilePassSemantics::EntryHelper::operator () (AST::Initialization& initialization)
{
	self->StateStack.push(CompilePassSemantics::STATE_STATEMENT);

	StringHandle type = self->CurrentProgram->AddString(std::wstring(initialization.TypeSpecifier.begin(), initialization.TypeSpecifier.end()));
	StringHandle lhs = self->CurrentProgram->AddString(std::wstring(initialization.LHS.begin(), initialization.LHS.end()));

	self->CurrentStatements.push_back(new IRSemantics::Statement(type, initialization.TypeSpecifier));
	std::auto_ptr<IRSemantics::Expression> param(new IRSemantics::Expression());
	param->AddAtom(new IRSemantics::ExpressionAtomIdentifier(lhs, initialization.LHS));
	self->CurrentStatements.back()->AddParameter(param.release());
}

//
// Finish traversing a node corresponding to an initialization
//
void CompilePassSemantics::ExitHelper::operator () (AST::Initialization&)
{
	self->StateStack.pop();
	
	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockStatementEntry(self->CurrentStatements.back()));
		self->CurrentStatements.pop_back();
		break;

	case CompilePassSemantics::STATE_FUNCTION_RETURN:
		{
			std::auto_ptr<IRSemantics::Expression> expression(new IRSemantics::Expression);
			expression->AddAtom(new IRSemantics::ExpressionAtomStatement(self->CurrentStatements.back()));
			self->CurrentStatements.pop_back();
			self->CurrentFunctions.back()->SetReturnExpression(expression.release());
		}
		break;

	default:
		//
		// This is probably a missing language feature.
		//
		// An initialization AST node has been traversed but
		// its surrounding context was not explicitly handled
		// by one of the above cases. Ensure the AST generation
		// is correct and that the context is supported.
		//
		throw InternalException("Initialization occurs in an unrecognized context");
	}
}


//
// Begin traversing a node representing an entity invocation
//
void CompilePassSemantics::EntryHelper::operator () (AST::Entity& entity)
{
	StringHandle entityname = self->CurrentProgram->AddString(std::wstring(entity.Identifier.begin(), entity.Identifier.end()));

	self->StateStack.push(CompilePassSemantics::STATE_ENTITY);
	self->CurrentEntities.push_back(new IRSemantics::Entity(entityname, entity.Identifier));
}

//
// Finish traversing a node representing an entity invocation
//
void CompilePassSemantics::ExitHelper::operator () (AST::Entity&)
{
	self->StateStack.pop();
	
	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockEntityEntry(self->CurrentEntities.back()));
		self->CurrentEntities.pop_back();
		break;

	default:
		//
		// This is a malformed AST.
		//
		// Entities must always appear inside a code block of some kind.
		// This particular entity node was provided in some other context,
		// which is not supported. Check the traverser state stack to see
		// what went wrong.
		//
		throw InternalException("Entity provided in invalid context");
	}
}

//
// Begin traversing a node containing a chained entity invocation
//
void CompilePassSemantics::EntryHelper::operator () (AST::ChainedEntity& entity)
{
	StringHandle entityname = self->CurrentProgram->AddString(std::wstring(entity.Identifier.begin(), entity.Identifier.end()));

	self->StateStack.push(CompilePassSemantics::STATE_CHAINED_ENTITY);
	self->CurrentChainedEntities.push_back(new IRSemantics::Entity(entityname, entity.Identifier));
}

//
// Finish traversing a node containing a chained entity invocation
//
void CompilePassSemantics::ExitHelper::operator () (AST::ChainedEntity&)
{
	self->StateStack.pop();
	
	if(self->StateStack.top() != CompilePassSemantics::STATE_ENTITY)
	{
		//
		// This is a malformed AST.
		//
		// Chained entities must always appear as child nodes of a parent
		// entity so that the entirety of the chain can be linked back to
		// the correct entry/invocation logic. The AST node for a chained
		// entity was provided in some other context; examine the context
		// stack to see what went wrong.
		//
		throw InternalException("Chained entity found without an opening entity");
	}

	self->CurrentEntities.back()->AddChain(self->CurrentChainedEntities.back());
	self->CurrentChainedEntities.pop_back();
}

//
// Begin traversing a postfix entity invocation node
//
void CompilePassSemantics::EntryHelper::operator () (AST::PostfixEntity& entity)
{
	StringHandle entityname = self->CurrentProgram->AddString(std::wstring(entity.Identifier.begin(), entity.Identifier.end()));

	self->StateStack.push(CompilePassSemantics::STATE_POSTFIX_ENTITY);
	self->CurrentPostfixEntities.push_back(new IRSemantics::Entity(entityname, entity.Identifier));
}

//
// Finish traversing a postfix entity invocation node
//
void CompilePassSemantics::ExitHelper::operator () (AST::PostfixEntity&)
{
	self->StateStack.pop();
	
	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockEntityEntry(self->CurrentPostfixEntities.back()));
		self->CurrentPostfixEntities.pop_back();
		break;

	default:
		//
		// This is a malformed AST.
		//
		// Entities must always appear inside a code block of some kind.
		// This particular entity node was provided in some other context,
		// which is not supported. Check the traverser state stack to see
		// what went wrong.
		//
		throw InternalException("Postfix entity provided in invalid context");
	}
}


//
// Traverse a node containing an identifier
//
// Note that this is not necessarily always a leaf node; identifiers can
// be attached to entity invocations, function calls, etc. and therefore
// this needs to be able to handle all situations in which an identifier
// might appear.
//
void CompilePassSemantics::EntryHelper::operator () (AST::IdentifierT& identifier)
{
	Substring<positertype> raw(identifier.begin(), identifier.end());

	std::auto_ptr<IRSemantics::ExpressionAtom> iratom(NULL);

	if(raw.length() >= 2 && *raw.begin() == L'\"' && *raw.rbegin() == L'\"')
	{
		StringHandle handle = self->CurrentProgram->AddString(StripQuotes(raw));
		iratom.reset(new IRSemantics::ExpressionAtomLiteralString(handle));
	}
	else if(raw == L"true")
		iratom.reset(new IRSemantics::ExpressionAtomLiteralBoolean(true));
	else if(raw == L"false")
		iratom.reset(new IRSemantics::ExpressionAtomLiteralBoolean(false));
	else if(raw.find(L'.') != std::wstring::npos)
	{
		Real32 literalfloat;

		std::wistringstream convert(raw);
		if(!(convert >> literalfloat))
			throw std::runtime_error("Invalid floating point literal");
		
		iratom.reset(new IRSemantics::ExpressionAtomLiteralReal32(literalfloat));
	}
	else
	{
		if(raw.length() > 2 && (*raw.begin() == L'0') && (*(raw.begin() + 1) == L'x'))
		{
			UInteger32 literalint;

			std::wstring hexstr(raw);
			hexstr = hexstr.substr(2);
			std::wistringstream convert(hexstr);
			convert >> std::hex;
			if(convert >> literalint)
				iratom.reset(new IRSemantics::ExpressionAtomLiteralInteger32(static_cast<Integer32>(literalint)));
			else
				throw std::runtime_error("Bad hex");
		}
		else
		{
			UInteger32 literalint;

			std::wistringstream convert(raw);
			if(convert >> literalint)
				iratom.reset(new IRSemantics::ExpressionAtomLiteralInteger32(static_cast<Integer32>(literalint)));
			else
			{
				StringHandle handle = self->CurrentProgram->AddString(raw);
				iratom.reset(new IRSemantics::ExpressionAtomIdentifier(handle, identifier));
			}
		}
	}

	if(!iratom.get())
	{
		//
		// The token couldn't be parsed as any recognizable thing.
		//
		// This could have a few causes: lexer breakdowns, AST traversal
		// mistakes, or incomplete feature implementations. Examine the
		// contents of the raw string that was being processed, and see
		// what it corresponds to in the input program.
		//
		throw InternalException("Unrecognized token");
	}

	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_ASSIGNMENT:
		{
			std::auto_ptr<IRSemantics::Expression> irexpression(new IRSemantics::Expression());
			irexpression->AddAtom(iratom.release());
			self->CurrentAssignments.back()->SetRHSRecursive(new IRSemantics::AssignmentChainExpression(irexpression.release()));
		}
		break;

	case CompilePassSemantics::STATE_EXPRESSION_COMPONENT_PREFIXES:
		self->CurrentExpressions.back()->AddAtom(new IRSemantics::ExpressionAtomOperator(dynamic_cast<IRSemantics::ExpressionAtomIdentifier*>(iratom.get())->GetIdentifier(), false, 0));
		break;

	case CompilePassSemantics::STATE_EXPRESSION_COMPONENT:
	case CompilePassSemantics::STATE_EXPRESSION_FRAGMENT:
		self->CurrentExpressions.back()->AddAtom(iratom.release());
		break;

	case CompilePassSemantics::STATE_FUNCTION:
		self->CurrentFunctions.back()->SetName(self->CurrentProgram->AddString(raw));
		break;

	case CompilePassSemantics::STATE_FUNCTION_SIGNATURE_PARAMS:
		self->CurrentFunctionSignatures.back()->AddParam(self->CurrentProgram->AddString(raw));
		break;

	case CompilePassSemantics::STATE_FUNCTION_SIGNATURE_RETURN:
		self->CurrentFunctionSignatures.back()->SetReturnType(self->CurrentProgram->AddString(raw));
		break;

	case CompilePassSemantics::STATE_STRUCTURE_FUNCTION_PARAMS:
		self->CurrentStructureFunctions.back()->AddParam(self->CurrentProgram->AddString(raw), &identifier);
		break;

	case CompilePassSemantics::STATE_STRUCTURE_FUNCTION_RETURN:
		self->CurrentStructureFunctions.back()->SetReturnTypeName(self->CurrentProgram->AddString(raw), &identifier);
		break;

	case CompilePassSemantics::STATE_POSTFIX_ENTITY:
		self->CurrentPostfixEntities.back()->SetPostfixIdentifier(self->CurrentProgram->AddString(raw));
		break;

	case CompilePassSemantics::STATE_FUNCTION_TAG:
		self->CurrentFunctionTags.back()->TagName = self->CurrentProgram->AddString(raw);
		self->CurrentFunctionTags.back()->OriginalTag = identifier;
		self->StateStack.push(CompilePassSemantics::STATE_FUNCTION_TAG_PARAM);
		break;

	case CompilePassSemantics::STATE_FUNCTION_TAG_PARAM:
		self->CurrentFunctionTags.back()->Parameters.push_back(iratom->ConvertToCompileTimeParam(*self->CurrentNamespace));
		break;

	case CompilePassSemantics::STATE_SUM_TYPE:
		self->CurrentNamespace->Types.SumTypes.AddBaseTypeToSumType(self->CurrentSumType, self->CurrentProgram->AddString(raw));
		break;

	case CompilePassSemantics::STATE_TEMPLATE_ARGS:
		self->CurrentTemplateArgs.back()->push_back(iratom->ConvertToCompileTimeParam(*self->CurrentNamespace));
		break;

	default:
		//
		// This is likely either a malformed AST or a bug in the traverser.
		//
		// A token of some flavor was provided, but the context does not link
		// to any situation we recognize. Check to see what the context stack
		// contains and make sure any appropriate language features are being
		// handled correctly above.
		//
		throw InternalException("Token appears in unrecognized context");
	}
}


//
// Traverse a node representing a function reference signature; this is
// typically used when passing function references to higher-order functions
// in the program being compiled.
//
// Note that function reference signatures are not leaf nodes!
//
void CompilePassSemantics::EntryHelper::operator () (AST::FunctionReferenceSignature&)
{
	if(self->CurrentFunctions.empty())
	{
		//
		// Either the AST is malformed, or a feature is missing.
		//
		// Function signatures are expected to only appear in certain
		// contexts (e.g. function definitions) and the AST traverser
		// handed us one in some other situation. Examine the context
		// state stack to see what's going on.
		//
		throw InternalException("Function signature provided in unrecognized context");
	}

	self->StateStack.push(CompilePassSemantics::STATE_FUNCTION_SIGNATURE);

	self->CurrentFunctionSignatures.push_back(new IRSemantics::FunctionParamFuncRef);
}

void CompilePassSemantics::ExitHelper::operator () (AST::FunctionReferenceSignature& refsig)
{
	StringHandle name = self->CurrentProgram->AddString(std::wstring(refsig.Identifier.begin(), refsig.Identifier.end()));
	self->CurrentFunctions.back()->AddParameter(name, self->CurrentFunctionSignatures.back(), false, self->Errors);
	self->CurrentFunctionSignatures.pop_back();
	self->StateStack.pop();
}



void CompilePassSemantics::EntryHelper::operator () (AST::TypeAlias& alias)
{
	StringHandle aliasname = self->CurrentProgram->AddString(std::wstring(alias.AliasName.begin(), alias.AliasName.end()));

	if(self->CurrentNamespace->Types.GetTypeByName(aliasname) != Metadata::EpochType_Error)
	{
		self->Errors.SetContext(alias.AliasName);
		self->Errors.SemanticError("A type with this name already exists");
		return;
	}

	StringHandle representationname = self->CurrentProgram->AddString(std::wstring(alias.RepresentationName.begin(), alias.RepresentationName.end()));
	Metadata::EpochTypeID representationtype = self->CurrentNamespace->Types.GetTypeByName(representationname);

	if(representationtype == Metadata::EpochType_Error)
	{
		self->Errors.SetContext(alias.RepresentationName);
		self->Errors.SemanticError("No type by this name was found");
		return;
	}

	self->CurrentNamespace->Types.Aliases.AddWeakAlias(aliasname, representationtype);
}


void CompilePassSemantics::EntryHelper::operator () (AST::StrongTypeAlias& alias)
{
	StringHandle aliasname = self->CurrentProgram->AddString(std::wstring(alias.AliasName.begin(), alias.AliasName.end()));

	if(self->CurrentNamespace->Types.GetTypeByName(aliasname) != Metadata::EpochType_Error)
	{
		self->Errors.SetContext(alias.AliasName);
		self->Errors.SemanticError("A type with this name already exists");
		return;
	}

	StringHandle representationname = self->CurrentProgram->AddString(std::wstring(alias.RepresentationName.begin(), alias.RepresentationName.end()));
	Metadata::EpochTypeID representationtype = self->CurrentNamespace->Types.GetTypeByName(representationname);

	if(representationtype == Metadata::EpochType_Error)
	{
		self->Errors.SetContext(alias.RepresentationName);
		self->Errors.SemanticError("No type by this name was found");
		return;
	}

	self->CurrentNamespace->Types.Aliases.AddStrongAlias(aliasname, representationtype, representationname);

	self->CurrentProgram->Session.CompileTimeHelpers.insert(std::make_pair(aliasname, &CompileConstructorHelper));

	Metadata::EpochTypeFamily family = Metadata::GetTypeFamily(representationtype);
	if(family == Metadata::EpochTypeFamily_Structure)
	{
		self->CurrentNamespace->Types.Structures.GetDefinition(representationname)->CompileTimeCodeExecution(representationname, *self->CurrentNamespace, self->Errors);
		representationname = self->CurrentNamespace->Types.Structures.GetConstructorName(representationname);
	}
	
	self->CurrentNamespace->Functions.SetSignature(aliasname, self->CurrentNamespace->Functions.GetSignature(representationname));
}

void CompilePassSemantics::EntryHelper::operator () (AST::SumType& sumtype)
{
	self->StateStack.push(CompilePassSemantics::STATE_SUM_TYPE);
	self->CurrentSumType = self->CurrentNamespace->Types.SumTypes.Add(std::wstring(sumtype.SumTypeName.begin(), sumtype.SumTypeName.end()), self->Errors);
	
	std::wstring firstbaseraw(sumtype.FirstBaseType.TypeName.begin(), sumtype.FirstBaseType.TypeName.end());
	self->CurrentNamespace->Types.SumTypes.AddBaseTypeToSumType(self->CurrentSumType, self->CurrentProgram->AddString(firstbaseraw));
}

void CompilePassSemantics::ExitHelper::operator () (AST::SumType&)
{
	self->StateStack.pop();
}


void CompilePassSemantics::EntryHelper::operator () (AST::Nothing&)
{
	if(self->StateStack.top() == CompilePassSemantics::STATE_FUNCTION_PARAM)
	{
		if(self->CurrentFunctions.empty())
		{
			//
			// This is a failure of the AST traversal.
			//
			// A function parameter definition node has been
			// traversed outside the context of a function
			// definition.
			//
			// Examine the AST generation and traversal logic.
			//
			throw InternalException("Attempted to traverse a function parameter definition AST node (\"nothing\") in an invalid context");
		}


		std::wostringstream formatter;
		formatter << L"nothing" << ++self->StupidCounter;
		StringHandle name = self->CurrentProgram->AddString(formatter.str());
		StringHandle type = self->CurrentProgram->FindString(L"nothing");

		std::auto_ptr<IRSemantics::FunctionParamNamed> irparam(new IRSemantics::FunctionParamNamed(type, CompileTimeParameterVector(), false));
		self->CurrentFunctions.back()->AddParameter(name, irparam.release(), true, self->Errors);
	}
	else
		throw InternalException("Unexpected 'nothing'");
}


void CompilePassSemantics::EntryHelper::operator () (AST::TemplateParameter& param)
{
	std::wstring metatype(param.Type.begin(), param.Type.end());
	if(metatype != L"type")
		throw NotImplementedException("This form of genericity is not implemented.");		// TODO - support more than just "type T" in template param lists

	StringHandle paramname = self->CurrentProgram->AddString(std::wstring(param.Name.begin(), param.Name.end()));
	
	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_STRUCTURE:
		self->CurrentStructures.back()->AddTemplateParameter(Metadata::EpochType_Wildcard, paramname);
		break;

	case CompilePassSemantics::STATE_FUNCTION:
		self->CurrentFunctions.back()->AddTemplateParameter(Metadata::EpochType_Wildcard, paramname);
		break;

	case CompilePassSemantics::STATE_SUM_TYPE:
		self->CurrentNamespace->Types.SumTypes.AddTemplateParameter(self->CurrentSumType, paramname);
		break;

	default:
		//
		// This is a discrepancy between the parser and the IR traverser.
		//
		// Something has been given a set of template parameters, but the IR
		// traverser doesn't know what to do with them. Chances are that the
		// code is in a transitional state where parser support for template
		// parameters has been added to some area of the syntax but the full
		// support has yet to be fleshed out throughout the compiler.
		//
		throw InternalException("Unrecognized context for template parameter");
	}
}


//
// Traverse a special marker that indicates the subsequent nodes belong
// to the return expression definition of a function
//
void CompilePassSemantics::EntryHelper::operator () (Markers::FunctionReturnExpression&)
{
	self->StateStack.push(CompilePassSemantics::STATE_FUNCTION_RETURN);
	self->InFunctionReturn = true;
}

//
// Finish traversing a function's return expression
//
void CompilePassSemantics::ExitHelper::operator () (Markers::FunctionReturnExpression&)
{
	if(self->CurrentFunctions.empty())
	{
		//
		// Almost certainly a bug in the AST generation or traversal.
		//
		// The "function return expression" marker should ONLY be traversed
		// when we are processing the return value expression of a function
		// definition; any other situation is technically bogus. Check into
		// the current traverser state stack to see what's going on.
		//
		throw InternalException("Function return provided but no function is being processed");
	}

	if(!self->CurrentExpressions.empty())
	{
		self->CurrentFunctions.back()->SetReturnExpression(self->CurrentExpressions.back());
		self->CurrentExpressions.pop_back();
	}

	self->StateStack.pop();
	self->InFunctionReturn = false;
}


void CompilePassSemantics::EntryHelper::operator () (Markers::ExpressionComponentPrefixes&)
{
	self->StateStack.push(CompilePassSemantics::STATE_EXPRESSION_COMPONENT_PREFIXES);
}

void CompilePassSemantics::ExitHelper::operator () (Markers::ExpressionComponentPrefixes&)
{
	self->StateStack.pop();
}


void CompilePassSemantics::EntryHelper::operator () (Markers::FunctionSignatureParams&)
{
	self->StateStack.push(CompilePassSemantics::STATE_FUNCTION_SIGNATURE_PARAMS);
}

void CompilePassSemantics::ExitHelper::operator () (Markers::FunctionSignatureParams&)
{
	self->StateStack.pop();
}

void CompilePassSemantics::EntryHelper::operator () (Markers::FunctionSignatureReturn&)
{
	self->StateStack.push(CompilePassSemantics::STATE_FUNCTION_SIGNATURE_RETURN);
}

void CompilePassSemantics::ExitHelper::operator () (Markers::FunctionSignatureReturn&)
{
	self->StateStack.pop();
}



void CompilePassSemantics::EntryHelper::operator () (Markers::StructureFunctionParams&)
{
	self->StateStack.push(CompilePassSemantics::STATE_STRUCTURE_FUNCTION_PARAMS);
}

void CompilePassSemantics::ExitHelper::operator () (Markers::StructureFunctionParams&)
{
	self->StateStack.pop();
}

void CompilePassSemantics::EntryHelper::operator () (Markers::StructureFunctionReturn&)
{
	self->StateStack.push(CompilePassSemantics::STATE_STRUCTURE_FUNCTION_RETURN);
}

void CompilePassSemantics::ExitHelper::operator () (Markers::StructureFunctionReturn&)
{
	self->StateStack.pop();
}



void CompilePassSemantics::EntryHelper::operator () (Markers::TemplateArgs&)
{
	self->StateStack.push(CompilePassSemantics::STATE_TEMPLATE_ARGS);
	self->CurrentTemplateArgs.push_back(new CompileTimeParameterVector);
}

void CompilePassSemantics::ExitHelper::operator () (Markers::TemplateArgs&)
{
	self->StateStack.pop();
	
	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_STATEMENT:
		if(!self->CurrentTemplateArgs.back()->empty())
			self->CurrentStatements.back()->SetTemplateArgsDeferred(*self->CurrentTemplateArgs.back());
		break;

	case CompilePassSemantics::STATE_FUNCTION_PARAM:
		self->CurrentTemplateArgsScratch = *self->CurrentTemplateArgs.back();
		break;

	case CompilePassSemantics::STATE_STRUCTURE:
		self->CurrentStructures.back()->SetMemberTemplateArgs(self->LastStructureMemberName, *self->CurrentTemplateArgs.back(), *self->CurrentNamespace, self->Errors);
		break;

	default:
		throw NotImplementedException("Template parameterization not handled in this situation yet");
	}
	
	delete self->CurrentTemplateArgs.back();
	self->CurrentTemplateArgs.pop_back();
}


void CompilePassSemantics::EntryHelper::operator () (AST::RefTag&)
{
	if(self->CurrentStructures.empty())
		self->IsParamRef = true;
	else
		self->CurrentStructures.back()->SetMemberIsReference(self->LastStructureMemberName);
}


size_t CompilePassSemantics::FindLine(const AST::IdentifierT& identifier) const
{
	size_t line = 1;
	size_t delta = identifier.begin() - SourceBegin;
	for(size_t offset = 0; offset < delta; ++offset)
	{
		if(*(SourceBegin + offset) == L'\n')
			++line;
	}

	return line;
}

size_t CompilePassSemantics::FindColumn(const AST::IdentifierT& identifier) const
{
	size_t column = 0;
	size_t delta = identifier.begin() - SourceBegin;
	for(size_t offset = 0; offset < delta; ++offset)
	{
		wchar_t c = *(SourceBegin + offset);
		if(c == L'\n')
			column = 1;
		else if(c == L'\t')
			column += 8;
		else
			++column;
	}

	return column;
}

std::wstring CompilePassSemantics::FindSource(const AST::IdentifierT& identifier) const
{
	size_t targetline = FindLine(identifier);
	size_t line = 1;
	size_t delta = identifier.begin() - SourceBegin;
	for(size_t offset = 0; offset < delta; ++offset)
	{
		if(*(SourceBegin + offset) == L'\n')
		{
			if(++line == targetline)
			{
				size_t term = SourceEnd - SourceBegin;
				size_t endoffset = offset;
				while(++endoffset < term)
				{
					if(*(SourceBegin + endoffset) == L'\n')
						break;
				}
				return std::wstring(SourceBegin + offset, SourceBegin + endoffset);
			}
		}
	}

	return L"<unknown>";
}

void CompilePassSemantics::UpdateContext(CompileErrors& errors) const
{
	if(ErrorContext)
		errors.SetLocation(Session.FileName, FindLine(*ErrorContext), FindColumn(*ErrorContext), FindSource(*ErrorContext));
}

void CompilePassSemantics::UpdateFromContext(CompileErrors& errors, const AST::IdentifierT& context) const
{
	errors.SetLocation(Session.FileName, FindLine(context), FindColumn(context), FindSource(context));
}
