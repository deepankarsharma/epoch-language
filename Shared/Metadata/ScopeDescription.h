//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for describing the contents of lexical scopes
//

#pragma once


// Dependencies
#include "Metadata/FunctionSignature.h"
#include "Metadata/CompileTimeParams.h"

#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IDTypes.h"

#include <string>
#include <vector>
#include <map>


enum VariableOrigin
{
	VARIABLE_ORIGIN_LOCAL,
	VARIABLE_ORIGIN_PARAMETER,
	VARIABLE_ORIGIN_RETURN
};


class ScopeDescription
{
// Construction
public:
	ScopeDescription()
		: ParentScope(NULL),
		  Hoisted(false)
	{ }

	explicit ScopeDescription(ScopeDescription* parentscope)
		: ParentScope(parentscope),
		  Hoisted(false)
	{ }

// Configuration interface
public:
	void AddVariable(const std::wstring& identifier, StringHandle identifierhandle, StringHandle typenamehandle, Metadata::EpochTypeID type, VariableOrigin origin);
	void PrependVariable(const std::wstring& identifier, StringHandle identifierhandle, StringHandle typenamehandle, Metadata::EpochTypeID type, VariableOrigin origin);

// Inspection interface
public:
	bool HasVariable(const std::wstring& identifier) const;
	bool HasVariable(StringHandle identifier) const;

	const std::wstring& GetVariableName(size_t index) const;
	StringHandle GetVariableNameHandle(size_t index) const;
	Metadata::EpochTypeID GetVariableTypeByID(StringHandle variableid) const;
	Metadata::EpochTypeID GetVariableTypeByIndex(size_t index) const;
	VariableOrigin GetVariableOrigin(size_t index) const;

	size_t GetVariableCount() const
	{ return Variables.size(); }

	size_t GetParameterCount() const;

	bool HasReturnVariable() const;
	Metadata::EpochTypeID GetReturnVariableType() const;

	void Fixup(const std::vector<std::pair<StringHandle, Metadata::EpochTypeID> >& templateparams, const CompileTimeParameterVector& templateargs, const CompileTimeParameterVector& templateargtypes);

	size_t FindVariable(StringHandle variableid, size_t& outnumframes) const;

// Optimization helpers
public:
	void HoistInto(ScopeDescription* target);

// Public properties
public:
	ScopeDescription* ParentScope;

// Internal helper structures
private:
	struct VariableEntry
	{
		// Constructor for convenience
		VariableEntry(const std::wstring& identifier, StringHandle identifierhandle, StringHandle typenamehandle, Metadata::EpochTypeID type, VariableOrigin origin)
			: Identifier(identifier),
			  IdentifierHandle(identifierhandle),
			  Type(type),
			  TypeName(typenamehandle),
			  Origin(origin)
		{ }

		std::wstring Identifier;
		StringHandle IdentifierHandle;
		Metadata::EpochTypeID Type;
		StringHandle TypeName;
		VariableOrigin Origin;
	};

// Internal tracking
private:
	typedef std::vector<VariableEntry> VariableVector;
	VariableVector Variables;
	bool Hoisted;
};
