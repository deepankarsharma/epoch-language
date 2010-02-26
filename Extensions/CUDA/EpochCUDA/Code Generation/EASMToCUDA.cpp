//
// The Epoch Language Project
// CUDA Interoperability Library
//
// EpochASM-to-CUDA compiler logic
//

#include "pch.h"

#include "Code Generation/EASMToCUDA.h"
#include "Code Generation/CompiledCodeManager.h"
#include "CUDA Wrapper/Naming.h"
#include "Utility/Strings.h"
#include "Serialization/SerializationTokens.h"

#include <algorithm>


using namespace Compiler;
using namespace Extensions;


//
// Construct and initialize a wrapper for a compile session
//
CompilationSession::CompilationSession(TemporaryFileWriter& codefile, std::list<Traverser::ScopeContents>& registeredvariables, Extensions::CompileSessionHandle sessionhandle)
	: SessionHandle(sessionhandle),
	  TabDepth(0),
	  RegisteredVariables(&registeredvariables),
	  TemporaryCodeFile(codefile),
	  PrototypeHeaderFile(NULL),
	  ExpectingFunctionReturns(false),
	  ExpectingFunctionParams(false),
	  ExpectingFunctionBlock(false),
	  ExpectingDoWhileBlock(false),
	  MarshalFloats(false),
	  MarshalInts(false),
	  MarshalFloatArrays(false),
	  MarshalIntArrays(false),
	  IgnoreIndentation(false)
{
}


CompilationSession::CompilationSession(TemporaryFileWriter& codefile, TemporaryFileWriter& prototypesheaderfile, Extensions::CompileSessionHandle sessionhandle)
	: SessionHandle(sessionhandle),
	  TabDepth(0),
	  RegisteredVariables(NULL),
	  TemporaryCodeFile(codefile),
	  PrototypeHeaderFile(&prototypesheaderfile),
	  ExpectingFunctionReturns(false),
	  ExpectingFunctionParams(false),
	  ExpectingFunctionBlock(false),
	  ExpectingDoWhileBlock(false),
	  MarshalFloats(false),
	  MarshalInts(false),
	  MarshalFloatArrays(false),
	  MarshalIntArrays(false),
	  IgnoreIndentation(false)
{
}


//
// Callback: register the contents of a lexical scope with the compiler
//
void CompilationSession::RegisterScope(bool toplevel, size_t numcontents, const Traverser::ScopeContents* contents)
{
	WriteScopeContents(toplevel, numcontents, contents);
}

void CompilationSession::WriteScopeContents(bool toplevel, size_t numcontents, const Traverser::ScopeContents* contents)
{
	if(ExpectingFunctionReturns)
	{
		TemporaryCodeFile.OutputStream << L"__device__ ";
		if(PrototypeHeaderFile)
			PrototypeHeaderFile->OutputStream << L"__device__ ";

		if(numcontents == 0)
		{
			TemporaryCodeFile.OutputStream << L"void ";
			if(PrototypeHeaderFile)
				PrototypeHeaderFile->OutputStream << L"void ";
		}
		else if(numcontents > 1)
			throw std::exception("Functions with multiple returns cannot be invoked from a CUDA code segment");
		else
		{
			switch(contents->Type)
			{
			case VM::EpochVariableType_Integer:
				TemporaryCodeFile.OutputStream << L"int ";
				if(PrototypeHeaderFile)
					PrototypeHeaderFile->OutputStream << L"int ";
				break;
			case VM::EpochVariableType_Real:
				TemporaryCodeFile.OutputStream << L"float ";
				if(PrototypeHeaderFile)
					PrototypeHeaderFile->OutputStream << L"float ";
				break;

			default:
				throw std::exception("Cannot return this type from a function invoked via a CUDA code segment");
			}
		}

		PendingFunctionReturnValueName = contents->Identifier;
		PendingFunctionReturnValueType = contents->Type;

		ExpectingFunctionReturns = false;
		return;
	}

	if(ExpectingFunctionParams)
	{
		TemporaryCodeFile.OutputStream << PendingFunctionName << L"(";
		if(PrototypeHeaderFile)
			PrototypeHeaderFile->OutputStream << PendingFunctionName << L"(";

		for(size_t i = 0; i < numcontents; ++i)
		{
			switch(contents[i].Type)
			{
			case VM::EpochVariableType_Integer:
				TemporaryCodeFile.OutputStream << L"int ";
				if(PrototypeHeaderFile)
					PrototypeHeaderFile->OutputStream << L"int ";
				break;
			case VM::EpochVariableType_Real:
				TemporaryCodeFile.OutputStream << L"float ";
				if(PrototypeHeaderFile)
					PrototypeHeaderFile->OutputStream << L"float ";
				break;

			default:
				throw std::exception("Cannot pass this type to a function invoked via a CUDA code segment");
			}

			TemporaryCodeFile.OutputStream << contents[i].Identifier;
			if(PrototypeHeaderFile)
				PrototypeHeaderFile->OutputStream << contents[i].Identifier;

			if(i < numcontents - 1)
			{
				TemporaryCodeFile.OutputStream << L", ";
				if(PrototypeHeaderFile)
					PrototypeHeaderFile->OutputStream << L", ";
			}
		}

		TemporaryCodeFile.OutputStream << L")\n";
		if(PrototypeHeaderFile)
			PrototypeHeaderFile->OutputStream << L");\n";

		ExpectingFunctionParams = false;
		return;
	}


	if(!numcontents)
		return;

	if(toplevel)
	{
		for(size_t i = 0; i < numcontents; ++i)
		{
			switch(contents[i].Type)
			{
			case VM::EpochVariableType_Integer:				MarshalInts = true;			break;
			case VM::EpochVariableType_Real:				MarshalFloats = true;		break;
			case VM::EpochVariableType_Array:
				{
					switch(contents[i].ContainedType)
					{
					case VM::EpochVariableType_Integer:		MarshalIntArrays = true;	break;
					case VM::EpochVariableType_Real:		MarshalFloatArrays = true;	break;
					}
					break;
				}
			}
		}

		if(MarshalFloats)
		{
			PadTabs();
			TemporaryCodeFile.OutputStream << L"unsigned int __marshal_float_index = 0;\n";
		}

		if(MarshalInts)
		{
			PadTabs();
			TemporaryCodeFile.OutputStream << L"unsigned int __marshal_int_index = 0;\n";
		}

		if(MarshalFloatArrays)
		{
			PadTabs();
			TemporaryCodeFile.OutputStream << L"unsigned int __marshal_float_array_index = 0;\n";
		}

		if(MarshalIntArrays)
		{
			PadTabs();
			TemporaryCodeFile.OutputStream << L"unsigned int __marshal_int_array_index = 0;\n";
		}
	}

	PadTabs();
	TemporaryCodeFile.OutputStream << L"// Define variables in the current scope\n";

	for(size_t i = 0; i < numcontents; ++i)
	{
		bool appendsemicolon = true;

		if(toplevel)
			RegisteredVariables->push_back(contents[i]);

		PadTabs();
		switch(contents[i].Type)
		{
		case VM::EpochVariableType_Integer:
			TemporaryCodeFile.OutputStream << L"int " << contents[i].Identifier;
			if(toplevel)
				TemporaryCodeFile.OutputStream << L" = __marshal_input_ints[__marshal_int_index++]";
			break;

		case VM::EpochVariableType_Real:
			TemporaryCodeFile.OutputStream << L"float " << contents[i].Identifier;
			if(toplevel)
				TemporaryCodeFile.OutputStream << L" = __marshal_input_floats[__marshal_float_index++]";
			break;

		case VM::EpochVariableType_Array:
			{
				VM::EpochVariableTypeID arraytype = contents[i].ContainedType;
				switch(arraytype)
				{
				case VM::EpochVariableType_Integer:
					TemporaryCodeFile.OutputStream << L"int ";
					break;

				case VM::EpochVariableType_Real:
					TemporaryCodeFile.OutputStream << L"float ";
					break;

				default:
					throw std::exception("Cannot pass arrays of this type");
				}

				TemporaryCodeFile.OutputStream << contents[i].Identifier << L"[" << contents[i].ContainedSize << L"];\n";
				ArraySizeCache[contents[i].Identifier] = contents[i].ContainedSize;

				PadTabs();
				TemporaryCodeFile.OutputStream << "for(unsigned __marshal_array_counter = 0; __marshal_array_counter < " << contents[i].ContainedSize << "; ++__marshal_array_counter)\n";
				++TabDepth;
				PadTabs();
				TemporaryCodeFile.OutputStream << contents[i].Identifier << L"[__marshal_array_counter] = ";
				
				switch(arraytype)
				{
				case VM::EpochVariableType_Integer:
					TemporaryCodeFile.OutputStream << L"__marshal_input_int_arrays[__marshal_int_array_index++];\n";
					break;

				case VM::EpochVariableType_Real:
					TemporaryCodeFile.OutputStream << L"__marshal_input_float_arrays[__marshal_float_array_index++];\n";
					break;

				default:
					throw std::exception("Cannot pass arrays of this type");
				}				
				
				--TabDepth;
				appendsemicolon = false;
			}
			break;

		default:
			throw std::exception("Scope contains a variable of an unrecognized type; cannot generate declaration/initialization code");
		}

		if(appendsemicolon)
			TemporaryCodeFile.OutputStream << L";\n";
	}

	TemporaryCodeFile.OutputStream << L"\n";
}



//
// Callback: register a leaf (AKA an EpochASM instruction) with the compiler
//
void CompilationSession::RegisterLeaf(const wchar_t* token, const Traverser::Payload* payload)
{
	LeafStack.top().Leaves.push_back(Leaf(token, payload));
}


//
// Callback: enter a lexical scope/nested block
//
void CompilationSession::EnterNode()
{
	if(!LeafStack.empty())
	{
		WriteLeaves(LeafStack.top().Leaves, true);
		LeafStack.top().Leaves.clear();
	}

	LeafStack.push(LeafBlock(ExpectingFunctionBlock, ExpectingDoWhileBlock));

	PadTabs();
	TemporaryCodeFile.OutputStream << L"{\n";
	++TabDepth;

	if(ExpectingFunctionBlock)
	{
		ExpectingFunctionBlock = false;

		PadTabs();
		switch(PendingFunctionReturnValueType)
		{
		case VM::EpochVariableType_Integer:			TemporaryCodeFile.OutputStream << L"int ";		break;
		case VM::EpochVariableType_Real:			TemporaryCodeFile.OutputStream << L"float ";	break;

		default:
			throw std::exception("Cannot return this type from a function invoked via a CUDA code segment");
		}

		TemporaryCodeFile.OutputStream << PendingFunctionReturnValueName << L";\n";
	}

	ExpectingDoWhileBlock = false;
}

//
// Callback: exit a lexical scope/nested block
//
void CompilationSession::ExitNode()
{
	if(LeafStack.top().IsDoWhile)
	{
		LeafList conditionleaves = PopTrailingLeaves(LeafStack.top().Leaves);
		WriteLeaves(LeafStack.top().Leaves, true);
		--TabDepth;
		PadTabs();
		TemporaryCodeFile.OutputStream << L"} while(";
		bool oldvalue = IgnoreIndentation;
		IgnoreIndentation = true;
		WriteLeaves(conditionleaves, false);
		IgnoreIndentation = oldvalue;
		TemporaryCodeFile.OutputStream << L");\n";
	}
	else
	{
		WriteLeaves(LeafStack.top().Leaves, true);

		if(LeafStack.top().IsFunction)
		{
			PadTabs();
			TemporaryCodeFile.OutputStream << L"return " << PendingFunctionReturnValueName << L";\n";
		}

		--TabDepth;
		PadTabs();
		TemporaryCodeFile.OutputStream << L"}\n";
	}

	LeafStack.pop();
}


//
// Helper routine for converting traversal payload data to code
//
// Note that we do not use an overloaded << operator for outputting payload structs
// to std streams; this is because we use the EpochVariableType_Error type as a
// sentinel to indicate that the payload should output the contents of a temporary
// stream buffer used to assemble infix expressions. Since this is fairly special
// behaviour and only makes sense within the context of our compiler, it is cleaner
// to use this output wrapper than try to generalize << operator support accordingly.
//
void CompilationSession::OutputPayload(const Traverser::Payload& payload, std::wostream& stream)
{
	switch(payload.Type)
	{
	case VM::EpochVariableType_Error:			stream << ExpressionConstruction.str(); ExpressionConstruction.str(L"");	break;
	case VM::EpochVariableType_Integer:			stream << payload.Int32Value;												break;
	case VM::EpochVariableType_Real:			stream << payload.FloatValue;												break;
	case VM::EpochVariableType_String:
		if(payload.IsIdentifier)
			stream << payload.StringValue;
		else
			stream << "\"" << payload.StringValue << "\"";
		break;
	default:
		throw std::exception("Cannot emit contents of Payload structure - type is not supported or not recognized");
	}
}

//
// Helper function for making code look all pretty and tabified
//
void CompilationSession::PadTabs()
{
	if(IgnoreIndentation)
		return;

	for(unsigned i = 0; i < TabDepth; ++i)
		TemporaryCodeFile.OutputStream << L"\t";
}

//
// Helper function for writing the beginning of a function definition
//
// This code includes preparatory steps for marshalling Epoch data into CUDA variables
//
void CompilationSession::FunctionPreamble(Extensions::OriginalCodeHandle handle)
{
	PadTabs();
	TemporaryCodeFile.OutputStream << L"extern \"C\" __global__ void " << widen(GenerateFunctionName(handle)) << L"(float* __marshal_input_floats, int* __marshal_input_ints, float* __marshal_input_float_arrays, int* __marshal_input_int_arrays)\n";
	PadTabs();
	TemporaryCodeFile.OutputStream << L"{\n";
	++TabDepth;
	PadTabs();
	TemporaryCodeFile.OutputStream << L"// Copy variable values from the host into local variables\n";
}

//
// Helper function for generating CUDA-to-Epoch variable marshalling code
//
// Marshalling code is responsible for ensuring that the CUDA environment
// returns data to the Epoch environment seamlessly, allowing users to
// modify variables in CUDA code and see the results reflected in the calling
// Epoch code.
//
void CompilationSession::MarshalOut()
{
	TemporaryCodeFile.OutputStream << L"\n";
	PadTabs();
	TemporaryCodeFile.OutputStream << L"// Copy final variable values back into the data buffer for retrieval by the host\n";
	
	if(MarshalFloats)
	{
		PadTabs();
		TemporaryCodeFile.OutputStream << L"__marshal_float_index = 0;\n";
	}

	if(MarshalInts)
	{
		PadTabs();
		TemporaryCodeFile.OutputStream << L"__marshal_int_index = 0;\n";
	}

	if(MarshalFloatArrays)
	{
		PadTabs();
		TemporaryCodeFile.OutputStream << L"__marshal_float_array_index = 0;\n";
	}

	if(MarshalIntArrays)
	{
		PadTabs();
		TemporaryCodeFile.OutputStream << L"__marshal_int_array_index = 0;\n";
	}

	for(std::list<Traverser::ScopeContents>::const_iterator iter = RegisteredVariables->begin(); iter != RegisteredVariables->end(); ++iter)
	{
		PadTabs();
		switch(iter->Type)
		{
		case VM::EpochVariableType_Integer:
			TemporaryCodeFile.OutputStream << L"__marshal_input_ints[__marshal_int_index++] = " << iter->Identifier << L";\n";
			break;

		case VM::EpochVariableType_Real:
			TemporaryCodeFile.OutputStream << L"__marshal_input_floats[__marshal_float_index++] = " << iter->Identifier << L";\n";
			break;

		case VM::EpochVariableType_Array:
			{
				switch(iter->ContainedType)
				{
				case VM::EpochVariableType_Integer:
					TemporaryCodeFile.OutputStream << "for(unsigned __marshal_array_counter = 0; __marshal_array_counter < " << GetArraySize(iter->Identifier) << "; ++__marshal_array_counter)\n";
					++TabDepth;
					PadTabs();
					TemporaryCodeFile.OutputStream << "__marshal_input_int_arrays[__marshal_int_array_index++] = " << iter->Identifier << "[__marshal_array_counter];\n";
					--TabDepth;
					break;

				case VM::EpochVariableType_Real:
					break;

				default:
					throw std::exception("Unsupported type - cannot generate code to transfer variable data from the CUDA device!");
				}
			}
			break;

		default:
			throw std::exception("Unsupported type - cannot generate code to transfer variable data from the CUDA device!");
		}
	}

	--TabDepth;
	PadTabs();
	TemporaryCodeFile.OutputStream << L"}\n\n";
}


void CompilationSession::WriteLeaves(LeafList& leaves, bool leavesarestatements)
{
	std::reverse(leaves.begin(), leaves.end());

	std::list<std::wstring> OutputLines;

	for(LeafList::const_iterator iter = leaves.begin(); iter != leaves.end(); ++iter)
	{
		if(iter->Token == Serialization::PushOperation)
			continue;

		std::wstring line = GenerateLeafCode(iter);

		if(leavesarestatements && iter->Token != Serialization::DoWhile && (*line.rbegin() != L';'))
			line += L";";

		OutputLines.push_front(line);
	}

	for(std::list<std::wstring>::const_iterator iter = OutputLines.begin(); iter != OutputLines.end(); ++iter)
	{
		PadTabs();
		TemporaryCodeFile.OutputStream << (*iter);
		if(!IgnoreIndentation)
			TemporaryCodeFile.OutputStream << L"\n";
	}
}


std::wstring CompilationSession::GenerateLeafCode(LeafList::const_iterator& iter)
{
	// TODO - factor out into helper functions

	std::wostringstream out;

	if(iter->Token == Serialization::InitializeValue ||
	   iter->Token == Serialization::AssignValue)
	{
		OutputPayload(iter->Payload, out);
		out << L" = ";

		AdvanceLeafIterator(iter);
		out << GenerateLeafCode(iter);
		out << L";";
	
		return out.str();
	}
	else if(iter->Token == Serialization::PushIntegerLiteral ||
			iter->Token == Serialization::PushInteger16Literal ||
			iter->Token == Serialization::PushRealLiteral)
	{
		OutputPayload(iter->Payload, out);
		return out.str();
	}
	else if(iter->Token == Serialization::While)
	{
		return L"while(true)";
	}
	else if(iter->Token == Serialization::AddIntegers ||
			iter->Token == Serialization::AddInteger16s ||
			iter->Token == Serialization::AddReals)
	{
		AdvanceLeafIterator(iter);
		std::wstring operand = GenerateLeafCode(iter);
		AdvanceLeafIterator(iter);
		out << GenerateLeafCode(iter);
		out << L" + " << operand;
		return out.str();
	}
	else if(iter->Token == Serialization::SubtractIntegers ||
			iter->Token == Serialization::SubtractInteger16s ||
			iter->Token == Serialization::SubtractReals)
	{
		AdvanceLeafIterator(iter);
		std::wstring operand = GenerateLeafCode(iter);
		AdvanceLeafIterator(iter);
		out << GenerateLeafCode(iter);
		out << L" - " << operand;
		return out.str();
	}
	else if(iter->Token == Serialization::MultiplyIntegers ||
			iter->Token == Serialization::MultiplyInteger16s ||
			iter->Token == Serialization::MultiplyReals)
	{
		AdvanceLeafIterator(iter);
		std::wstring operand = GenerateLeafCode(iter);
		AdvanceLeafIterator(iter);
		out << GenerateLeafCode(iter);
		out << L" * " << operand;
		return out.str();
	}
	else if(iter->Token == Serialization::DivideIntegers ||
			iter->Token == Serialization::DivideInteger16s ||
			iter->Token == Serialization::DivideReals)
	{
		AdvanceLeafIterator(iter);
		std::wstring operand = GenerateLeafCode(iter);
		AdvanceLeafIterator(iter);
		out << GenerateLeafCode(iter);
		out << L" / " << operand;
		return out.str();
	}
	else if(iter->Token == Serialization::GetValue)
	{
		return iter->Payload.StringValue;
	}
	else if(iter->Token == Serialization::WhileCondition)
	{
		AdvanceLeafIterator(iter);
		out << L"if(!(" << GenerateLeafCode(iter) << L")) break;";
		return out.str();
	}
	else if(iter->Token == Serialization::IsLesser)
	{
		AdvanceLeafIterator(iter);
		std::wstring operand = GenerateLeafCode(iter);
		AdvanceLeafIterator(iter);
		out << GenerateLeafCode(iter);
		out << L" < " << operand;
		return out.str();
	}
	else if(iter->Token == Serialization::IsGreater)
	{
		AdvanceLeafIterator(iter);
		std::wstring operand = GenerateLeafCode(iter);
		AdvanceLeafIterator(iter);
		out << GenerateLeafCode(iter);
		out << L" > " << operand;
		return out.str();
	}
	else if(iter->Token == Serialization::ArrayLength)
	{
		out << GetArraySize(iter->Payload.StringValue);
		return out.str();
	}
	else if(iter->Token == Serialization::WriteArray)
	{
		std::wstring arrayidentifier = iter->Payload.StringValue;
		AdvanceLeafIterator(iter);
		std::wstring rhs = GenerateLeafCode(iter);
		AdvanceLeafIterator(iter);
		out << arrayidentifier << L"[";
		out << GenerateLeafCode(iter) << L"] = ";
		out << rhs << L";";
		return out.str();
	}
	else if(iter->Token == Serialization::ReadArray)
	{
		out << iter->Payload.StringValue << L"[";
		AdvanceLeafIterator(iter);
		out << GenerateLeafCode(iter) << L"]";
		return out.str();
	}
	else if(iter->Token == Serialization::Invoke)
	{
		std::wstring funcname = iter->Payload.StringValue;
		RecordInvokedFunction(SessionHandle, funcname);

		size_t paramcount = iter->Payload.ParameterCount;
		out << funcname << L"(";

		for(size_t i = 0; i < paramcount; ++i)
		{
			AdvanceLeafIterator(iter);
			out << GenerateLeafCode(iter);
		}

		out << L")";
		return out.str();
	}
	else if(iter->Token == Serialization::DoWhile)
	{
		ExpectingDoWhileBlock = true;
		return L"do";
	}
	
	throw std::exception("Cannot generate CUDA code for the given EASM instruction");
}


size_t CompilationSession::GetArraySize(const std::wstring& arrayname) const
{
	std::map<std::wstring, size_t>::const_iterator iter = ArraySizeCache.find(arrayname);
	if(iter != ArraySizeCache.end())
		return iter->second;

	throw std::exception("Cannot determine size of array variable - name not recognized");
}

void CompilationSession::AdvanceLeafIterator(LeafList::const_iterator& iter) const
{
	do
	{
		++iter;
	} while(iter->Token == Serialization::PushOperation);
}


void CompilationSession::ExpectFunctionTraversal(const std::wstring& functionname)
{
	PendingFunctionName = functionname;
	ExpectingFunctionReturns = ExpectingFunctionParams = ExpectingFunctionBlock = true;
}


CompilationSession::LeafList CompilationSession::PopTrailingLeaves(LeafList& leaves)
{
	LeafList ret;

	size_t paramcount = 1;

	do
	{
		while(leaves.rbegin()->Token == Serialization::PushOperation)
			leaves.pop_back();

		ret.push_back(*leaves.rbegin());
		paramcount += leaves.rbegin()->Payload.ParameterCount;
		--paramcount;
		leaves.pop_back();
	} while(paramcount > 0);

	std::reverse(ret.begin(), ret.end());
	return ret;
}

