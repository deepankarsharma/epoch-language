//
// The Epoch Language Project
// Shared Library Code
//
// Definitions for bytecode instruction values
//

#pragma once


namespace Bytecode
{

	typedef unsigned char Instruction;

	namespace Instructions
	{

		static const Instruction Halt = 0x00;
		static const Instruction NoOp = 0x01;

		static const Instruction Push = 0x02;
		static const Instruction Read = 0x03;

		static const Instruction Invoke = 0x04;
		static const Instruction Return = 0x05;

		static const Instruction BeginEntity = 0x06;
		static const Instruction EndEntity = 0x07;

		static const Instruction PoolString = 0x08;
		static const Instruction DefineLexicalScope = 0x09;

	}

}

