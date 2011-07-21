#include "pch.h"

#include "Parser/ExpressionGrammar.h"
#include "Parser/LiteralGrammar.h"

#include "Lexer/AdaptTokenDirective.h"


ExpressionGrammar::ExpressionGrammar(const Lexer::EpochLexerT& lexer, const LiteralGrammar& literalgrammar)
	: ExpressionGrammar::base_type(Expression)
{
	using namespace boost::spirit::qi;

	InfixSymbols.add(L".");

	MemberAccess %= lexer.StringIdentifier % lexer.Dot;

	PreOperatorStatement %= adapttokens[PreOperatorSymbols] >> MemberAccess;
	PostOperatorStatement %= MemberAccess >> adapttokens[PostOperatorSymbols];
	Parenthetical %= lexer.OpenParens >> (PreOperatorStatement | PostOperatorStatement | Expression) >> lexer.CloseParens;

	EntityParamsInner %= (Expression % lexer.Comma) >> lexer.CloseParens;
	EntityParamsEmpty = lexer.CloseParens;
	EntityParams %= lexer.OpenParens >> (EntityParamsEmpty | EntityParamsInner);
	Statement %= lexer.StringIdentifier >> EntityParams;

	Prefixes %= +adapttokens[PrefixSymbols];
	ExpressionChunk = (Parenthetical | literalgrammar | Statement | as<AST::IdentifierT>()[lexer.StringIdentifier]);
	ExpressionComponent %= ExpressionChunk | (Prefixes >> ExpressionChunk);
	ExpressionFragment %= (adapttokens[InfixSymbols] >> ExpressionComponent);
	Expression %= ExpressionComponent >> *ExpressionFragment;

	AssignmentOperator %= (lexer.Equals | adapttokens[OpAssignSymbols]);
	SimpleAssignment %= (as<AST::IdentifierT>()[lexer.StringIdentifier] >> AssignmentOperator >> ExpressionOrAssignment);
	MemberAssignment %= (MemberAccess >> AssignmentOperator >> ExpressionOrAssignment);
	Assignment %= SimpleAssignment | MemberAssignment;
	ExpressionOrAssignment %= Assignment | Expression;

	AnyStatement = Statement | PreOperatorStatement | PostOperatorStatement;
}

