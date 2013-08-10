//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for string-handling operators
//

#include "pch.h"

#include "Library Functionality/Operators/Strings.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"
#include "Utility/Strings.h"

#include "Runtime/Runtime.h"

#include "Metadata/Precedences.h"


extern Runtime::ExecutionContext* GlobalExecutionContext;


namespace
{

	StringHandle ConcatHandle = 0;
	StringHandle LengthHandle = 0;
	StringHandle NarrowStringHandle = 0;
	StringHandle WidenStringHandle = 0;
	StringHandle WidenBufferHandle = 0;

}


//
// Bind the library to a function metadata table
//
void StringLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"s1", Metadata::EpochType_String);
		signature.AddParameter(L"s2", Metadata::EpochType_String);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(ConcatHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"s", Metadata::EpochType_String);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(LengthHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"s", Metadata::EpochType_String);
		signature.SetReturnType(Metadata::EpochType_Buffer);
		AddToMapNoDupe(signatureset, std::make_pair(NarrowStringHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"p", Metadata::EpochType_Integer);
		signature.AddParameter(L"len", Metadata::EpochType_Integer);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(WidenStringHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"p", Metadata::EpochType_Buffer);
		signature.AddParameter(L"len", Metadata::EpochType_Integer);
		signature.SetReturnType(Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(WidenBufferHandle, signature));
	}
}

//
// Bind the library to the infix operator table
//
void StringLibrary::RegisterInfixOperators(StringSet& infixtable, PrecedenceTable& precedences)
{
	AddToSetNoDupe(infixtable, L";");
	AddToMapNoDupe(precedences, std::make_pair(ConcatHandle, PRECEDENCE_CONCATENATION));
}

void StringLibrary::PoolStrings(StringPoolManager& stringpool)
{
	ConcatHandle = stringpool.Pool(L";");
	LengthHandle = stringpool.Pool(L"length");
	NarrowStringHandle = stringpool.Pool(L"narrowstring");
	WidenStringHandle = stringpool.Pool(L"widenfromptr");
	WidenBufferHandle = stringpool.Pool(L"widenfrombuffer");
}

void StringLibrary::RegisterJITTable(JIT::JITTable& table)
{
	AddToMapNoDupe(table.LibraryExports, std::make_pair(NarrowStringHandle, "EpochLib_StrNarrow"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(ConcatHandle, "EpochLib_StrConcat"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(LengthHandle, "EpochLib_StrLen"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(WidenStringHandle, "EpochLib_StrWideFromPtr"));
	AddToMapNoDupe(table.LibraryExports, std::make_pair(WidenBufferHandle, "EpochLib_StrWideFromBuffer"));
}



extern "C" StringHandle EpochLib_StrConcat(StringHandle b, StringHandle a)
{
	std::wstring result = GlobalExecutionContext->GetPooledString(a) + GlobalExecutionContext->GetPooledString(b);
	return GlobalExecutionContext->PoolString(result);
}

extern "C" BufferHandle EpochLib_StrNarrow(StringHandle str)
{
	const std::wstring& sourcestring = GlobalExecutionContext->GetPooledString(str);

	std::string narrowed(narrow(sourcestring));

	BufferHandle destbuffer = GlobalExecutionContext->AllocateBuffer(narrowed.length() + 1);
	void* storage = GlobalExecutionContext->GetBuffer(destbuffer);
	memcpy(storage, narrowed.c_str(), narrowed.length());

	GlobalExecutionContext->TickBufferGarbageCollector();

	return destbuffer;
}

extern "C" StringHandle EpochLib_StrWideFromPtr(unsigned len, const char* p)
{
	return GlobalExecutionContext->PoolString(widen(std::string(p, len)));
}

extern "C" StringHandle EpochLib_StrWideFromBuffer(unsigned len, unsigned bufferhandle)
{
	return GlobalExecutionContext->PoolString(widen(std::string(reinterpret_cast<char*>(GlobalExecutionContext->GetBuffer(bufferhandle)), len)));
}

extern "C" size_t EpochLib_StrLen(StringHandle str)
{
	const std::wstring& sourcestring = GlobalExecutionContext->GetPooledString(str);
	return sourcestring.size();
}
