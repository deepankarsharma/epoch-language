//
// The Epoch Language Project
// Epoch Standard Library
//
// Function tag handlers for external (DLL-based) functions
//

#include "pch.h"

#include "Library Functionality/Function Tags/Externals.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Virtual Machine/Marshaling.h"


namespace
{

	//
	// Register a function tagged as an external
	//
	TagHelperReturn ExternalHelper(StringHandle functionname, const CompileTimeParameterVector& compiletimeparams, bool isprepass)
	{
		TagHelperReturn ret;

		if(!isprepass)
			ret.InvokeRuntimeFunction = L"@@external";
		else
		{
			ret.MetaTag = L"external";
			ret.MetaTagData.push_back(compiletimeparams[0].StringPayload);
			ret.MetaTagData.push_back(compiletimeparams[1].StringPayload);
		}

		return ret;
	}

}


//
// Bind the library to an execution dispatch table
//
void FunctionTags::RegisterExternalTag(FunctionSignatureSet& signatureset, StringPoolManager& stringpool)
{
	AddToMapNoDupe(signatureset, std::make_pair(stringpool.Pool(L"@@external"), FunctionSignature()));
}

//
// Bind the library to a function metadata table
//
void FunctionTags::RegisterExternalTag(EpochFunctionPtr marshalfunction, FunctionInvocationTable& table, StringPoolManager& stringpool)
{
	AddToMapNoDupe(table, std::make_pair(stringpool.Pool(L"@@external"), marshalfunction));
}


//
// Bind the library's tag helpers to the compiler
//
void FunctionTags::RegisterExternalTagHelper(FunctionTagHelperTable& table)
{
	AddToMapNoDupe(table, std::make_pair(L"external", ExternalHelper));
}

