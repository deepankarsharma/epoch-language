//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for constructing variables of built-in primitive types
//

#include "pch.h"

#include "Library Functionality/Type Constructors/Primitives.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"

#include "Compiler/CompileErrors.h"

#include "Runtime/Runtime.h"

#include "Libraries/LibraryJIT.h"

#include "Metadata/ScopeDescription.h"

#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"
#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"


namespace
{

	StringHandle IntegerHandle = 0;
	StringHandle Integer16Handle = 0;
	StringHandle RealHandle = 0;
	StringHandle BooleanHandle = 0;
	StringHandle StringTypeHandle = 0;
	StringHandle BufferTypeHandle = 0;
	StringHandle BufferCopyHandle = 0;
	StringHandle NothingHandle = 0;


	bool ConstructorJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* c = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::ConstantInt* cint = llvm::dyn_cast<llvm::ConstantInt>(c);
		size_t vartarget = static_cast<size_t>(cint->getValue().getLimitedValue());

		if(context.NameToIndexMap.find(vartarget) == context.NameToIndexMap.end())
			throw FatalException("Invalid binding target");

		reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateStore(p2, context.VariableMap[context.NameToIndexMap[vartarget]], false);

		return true;
	}

	bool ConstructorBufferJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* c = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::ConstantInt* cint = llvm::dyn_cast<llvm::ConstantInt>(c);
		size_t vartarget = static_cast<size_t>(cint->getValue().getLimitedValue());

		llvm::Value* bufferhandle = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateCall((*context.BuiltInFunctions)[JIT::JITFunc_Runtime_AllocBuffer], p2);

		reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateStore(bufferhandle, context.VariableMap[context.NameToIndexMap[vartarget]], false);

		return true;
	}

	bool ConstructorBufferCopyJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* p2 = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();
		llvm::Value* c = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::ConstantInt* cint = llvm::dyn_cast<llvm::ConstantInt>(c);
		size_t vartarget = static_cast<size_t>(cint->getValue().getLimitedValue());

		llvm::Value* bufferhandle = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateCall((*context.BuiltInFunctions)[JIT::JITFunc_Runtime_CopyBuffer], p2);

		reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateStore(bufferhandle, context.VariableMap[context.NameToIndexMap[vartarget]], false);

		return true;
	}
}


//
// Bind the library to a function metadata table
//
void TypeConstructors::RegisterLibraryFunctions(FunctionSignatureSet& signatureset)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier);
		signature.AddParameter(L"value", Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(IntegerHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier);
		signature.AddParameter(L"value", Metadata::EpochType_Integer16);
		AddToMapNoDupe(signatureset, std::make_pair(Integer16Handle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier);
		signature.AddParameter(L"value", Metadata::EpochType_String);
		AddToMapNoDupe(signatureset, std::make_pair(StringTypeHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier);
		signature.AddParameter(L"value", Metadata::EpochType_Boolean);
		AddToMapNoDupe(signatureset, std::make_pair(BooleanHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier);
		signature.AddParameter(L"value", Metadata::EpochType_Real);
		AddToMapNoDupe(signatureset, std::make_pair(RealHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier);
		signature.AddParameter(L"size", Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(BufferTypeHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier);
		signature.AddParameter(L"original", Metadata::EpochType_Buffer);
		AddToMapNoDupe(signatureset, std::make_pair(BufferCopyHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier);
		signature.AddPatternMatchedParameterIdentifier(NothingHandle);
		AddToMapNoDupe(signatureset, std::make_pair(NothingHandle, signature));
	}
}

//
// Bind the library to the compiler's internal semantic action table
//
void TypeConstructors::RegisterLibraryFunctions(FunctionCompileHelperTable& table)
{
	AddToMapNoDupe(table, std::make_pair(IntegerHandle, &CompileConstructorHelper));
	AddToMapNoDupe(table, std::make_pair(Integer16Handle, &CompileConstructorHelper));
	AddToMapNoDupe(table, std::make_pair(StringTypeHandle, &CompileConstructorHelper));
	AddToMapNoDupe(table, std::make_pair(BooleanHandle, &CompileConstructorHelper));
	AddToMapNoDupe(table, std::make_pair(RealHandle, &CompileConstructorHelper));
	AddToMapNoDupe(table, std::make_pair(BufferTypeHandle, &CompileConstructorHelper));
	AddToMapNoDupe(table, std::make_pair(NothingHandle, &CompileConstructorHelper));
	AddToMapNoDupe(table, std::make_pair(BufferCopyHandle, &CompileConstructorHelper));
}


void TypeConstructors::RegisterJITTable(JIT::JITTable& table)
{
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(IntegerHandle, &ConstructorJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(Integer16Handle, &ConstructorJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(RealHandle, &ConstructorJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(BooleanHandle, &ConstructorJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(StringTypeHandle, &ConstructorJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(BufferTypeHandle, &ConstructorBufferJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(BufferCopyHandle, &ConstructorBufferCopyJIT));
}


void TypeConstructors::PoolStrings(StringPoolManager& stringpool)
{
	IntegerHandle = stringpool.Pool(L"integer");
	Integer16Handle = stringpool.Pool(L"integer16");
	RealHandle = stringpool.Pool(L"real");
	BooleanHandle = stringpool.Pool(L"boolean");
	StringTypeHandle = stringpool.Pool(L"string");
	BufferTypeHandle = stringpool.Pool(L"buffer");
	NothingHandle = stringpool.Pool(L"nothing");
	BufferCopyHandle = stringpool.Pool(L"buffer@@copy");
}

void TypeConstructors::RegisterLibraryOverloads(OverloadMap& overloadmap)
{
	overloadmap[BufferTypeHandle].insert(BufferCopyHandle);
}
