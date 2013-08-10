//
// The Epoch Language Project
// EPOCHRUNTIME Runtime Library
//
// Exported routines comprising the EpochRuntime API
//

#include "pch.h"

#include "Runtime/Runtime.h"
#include "Runtime/GlobalContext.h"

#include "Utility/Strings.h"
#include "User Interface/Output.h"


#ifdef _MSC_VER
#define NORETURN __declspec(noreturn)
#else
#define NORETURN
#endif

#pragma warning(disable: 4702)			// unreachable code (we don't care)



extern "C" void STDCALL ExecuteByteCode(void* bytecodebuffer, size_t size);


namespace
{
	unsigned* TestHarness = NULL;


	typedef std::vector<char> DeferredExecBuffer;

	DWORD WINAPI ForkBytecodeExecThreadProc(void* param)
	{
		DeferredExecBuffer* buffer = reinterpret_cast<DeferredExecBuffer*>(param);

		ExecuteByteCode(&((*buffer)[0]), buffer->size());

		delete buffer;

		return 0;
	}
}


//
// Wrapper for executing a block of bytecode
//
// This is mainly in place to provide a C-compatible API to outside
// client code, as well as to prevent having to mess with throwing
// exceptions across DLL boundaries.
//
extern "C" void STDCALL ExecuteByteCode(void* bytecodebuffer, size_t size)
{
	try
	{
		Runtime::ExecutionContext context(reinterpret_cast<Bytecode::Instruction*>(bytecodebuffer), size, TestHarness);
		context.ExecuteByteCode();
	}
	catch(const std::exception& e)
	{
		UI::OutputStream out;
		out << UI::lightred << widen(e.what()).c_str() << UI::resetcolor << std::endl;
	}
	catch(...)
	{
		UI::OutputStream out;
		out << UI::lightred << L"Unknown exception" << UI::resetcolor << std::endl;
	}
}

extern "C" void STDCALL ExecuteByteCodeDeferred(void* bytecodebuffer, size_t size)
{
	DeferredExecBuffer* buffer = new DeferredExecBuffer(reinterpret_cast<char*>(bytecodebuffer), reinterpret_cast<char*>(bytecodebuffer) + size);

	HANDLE thread = ::CreateThread(NULL, 0, ForkBytecodeExecThreadProc, buffer, 0, NULL);
	::WaitForSingleObject(thread, INFINITE);
}

extern "C" void* STDCALL ExecuteByteCodePersistent(void* bytecodebuffer, size_t size)
{
	Runtime::ExecutionContext* context = new Runtime::ExecutionContext(reinterpret_cast<Bytecode::Instruction*>(bytecodebuffer), size, TestHarness);

	try
	{
		context->ExecuteByteCode();
	}
	catch(const std::exception& e)
	{
		UI::OutputStream out;
		out << UI::lightred << widen(e.what()).c_str() << UI::resetcolor << std::endl;
	}
	catch(...)
	{
		UI::OutputStream out;
		out << UI::lightred << "Unknown exception" << UI::resetcolor << std::endl;
	}

	return context;
}

extern "C" void STDCALL FreePersistedByteCode(Runtime::ExecutionContext* contextptr)
{
	delete contextptr;
}

//
// Link to a test harness for running unit tests
//
extern "C" void STDCALL LinkTestHarness(unsigned* harness)
{
	//try
	//{
		TestHarness = harness;
	//}
	//catch(...)
	//{
	//	::MessageBox(0, L"Failed to initialize test harness for Epoch Runtime", L"Epoch Internal Error", MB_ICONSTOP);
	//}
}

NORETURN extern "C" void Epoch_Halt()
{
	::MessageBox(0, L"Fatal error - program halted", L"Epoch Runtime", MB_ICONSTOP);
	std::terminate();
}


extern "C" void* Epoch_GetBuffer(BufferHandle handle)
{
	try
	{
		return Runtime::GetThreadContext()->GetBuffer(handle);
	}
	catch(...)
	{
		Epoch_Halt();
	}

	return 0;
}

extern "C" const wchar_t* Epoch_GetString(StringHandle handle)
{
	try
	{
		return Runtime::GetThreadContext()->GetPooledString(handle).c_str();
	}
	catch(...)
	{
		Epoch_Halt();
	}

	return 0;
}

extern "C" StringHandle Epoch_PoolString(const wchar_t* str)
{
	return Runtime::GetThreadContext()->PoolString(str);
}

extern "C" void* Epoch_AllocStruct(Metadata::EpochTypeID structtype)
{
	try
	{
		StructureHandle handle = Runtime::GetThreadContext()->AllocateStructure(Runtime::GetThreadContext()->GetStructureDefinition(structtype));
		Runtime::GetThreadContext()->TickStructureGarbageCollector();
		return handle;
	}
	catch(...)
	{
		Epoch_Halt();
	}

	return 0;
}

extern "C" void* Epoch_CopyStruct(StructureHandle handle)
{
	try
	{
		StructureHandle copyhandle = Runtime::GetThreadContext()->DeepCopy(handle);
		Runtime::GetThreadContext()->TickStructureGarbageCollector();
		return copyhandle;
	}
	catch(...)
	{
		Epoch_Halt();
	}

	return 0;
}

extern "C" void Epoch_Break()
{
	__debugbreak();
}

extern "C" BufferHandle Epoch_AllocBuffer(size_t size)
{
	try
	{
		BufferHandle handle = Runtime::GetThreadContext()->AllocateBuffer(size);
		Runtime::GetThreadContext()->TickBufferGarbageCollector();
		return handle;
	}
	catch(...)
	{
		Epoch_Halt();
	}

	return 0;
}

extern "C" BufferHandle Epoch_CopyBuffer(BufferHandle handle)
{
	try
	{
		BufferHandle clone = Runtime::GetThreadContext()->CloneBuffer(handle);
		Runtime::GetThreadContext()->TickBufferGarbageCollector();
		return clone;
	}
	catch(...)
	{
		Epoch_Halt();
	}

	return 0;
}

extern "C" BufferHandle Epoch_GetBufferByPtr(const char* bufferptr)
{
	try
	{
		return Runtime::GetThreadContext()->FindBuffer(bufferptr);
	}
	catch(...)
	{
		Epoch_Halt();
	}

	return 0;
}

extern "C" void FreeNativeCode()
{
	JIT::DestructLLVMNativeCode();
}
