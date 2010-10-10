//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Interface for binding a parsing session to a given set of semantic actions
//
// This interface allows us to gently decouple the parser itself from the set
// of code actions that the parsing process invokes. For example, we can bind
// a parser to a compiler, which emits executable bytecode; alternatively, we
// could bind a parser to a static analysis system for validation, and so on.
//

#pragma once


// Dependencies
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"
#include "Bytecode/EntityTags.h"

#include <boost/spirit/include/classic_position_iterator.hpp>


class SemanticActionInterface
{
public:
	virtual void Fail() = 0;

	virtual void SetPrepassMode(bool isprepass) = 0;

	virtual void StoreString(const std::wstring& name) = 0;
	virtual void StoreIntegerLiteral(Integer32 value) = 0;
	virtual void StoreStringLiteral(const std::wstring& value) = 0;
	virtual void StoreBooleanLiteral(bool value) = 0;
	virtual void StoreRealLiteral(Real32 value) = 0;

	virtual void StoreEntityType(Bytecode::EntityTag typetag) = 0;
	virtual void StoreEntityType(const std::wstring& identifier) = 0;
	virtual void StoreEntityCode() = 0;
	virtual void BeginEntityParams() = 0;
	virtual void CompleteEntityParams(bool ispostfixcloser) = 0;
	virtual void BeginEntityChain() = 0;
	virtual void EndEntityChain() = 0;
	virtual void StoreEntityPostfix(const std::wstring& identifier) = 0;
	virtual void InvokePostfixMetacontrol() = 0;

	virtual void StoreInfix(const std::wstring& identifier) = 0;
	virtual void PushInfixParam() = 0;
	virtual void CompleteInfix() = 0;
	virtual void FinalizeInfix() = 0;

	virtual void RegisterPreOperator(const std::wstring& identifier) = 0;
	virtual void RegisterPreOperand(const std::wstring& identifier) = 0;
	virtual void RegisterPostOperator(const std::wstring& identifier) = 0;
	virtual void RegisterPostOperand(const std::wstring& identifier) = 0;

	virtual void StoreUnaryPrefixOperator(const std::wstring& identifier) = 0;

	virtual void BeginParameterSet() = 0;
	virtual void EndParameterSet() = 0;
	virtual void RegisterParameterType(const std::wstring& type) = 0;
	virtual void RegisterParameterName(const std::wstring& name) = 0;
	virtual void RegisterPatternMatchedParameter() = 0;

	virtual void BeginReturnSet() = 0;
	virtual void EndReturnSet() = 0;
	virtual void RegisterReturnType(const std::wstring& type) = 0;
	virtual void RegisterReturnName(const std::wstring& name) = 0;
	virtual void RegisterReturnValue() = 0;

	virtual void BeginStatement(const std::wstring& statementname) = 0;
	virtual void BeginStatementParams() = 0;
	virtual void PushStatementParam() = 0;
	virtual void CompleteStatement() = 0;
	virtual void FinalizeStatement() = 0;

	virtual void BeginParenthetical() = 0;
	virtual void EndParenthetical() = 0;

	virtual void BeginAssignment() = 0;
	virtual void BeginOpAssignment(const std::wstring& identifier) = 0;
	virtual void CompleteAssignment() = 0;

	virtual void Finalize() = 0;

	virtual void EmitPendingCode() = 0;

	virtual void SanityCheck() const = 0;

	virtual void SetParsePosition(const boost::spirit::classic::position_iterator<const char*>& iterator) = 0;
};

