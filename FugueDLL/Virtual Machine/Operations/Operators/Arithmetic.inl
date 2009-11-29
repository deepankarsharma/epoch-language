//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Implementation of built-in operators and operations.
//

#include "pch.h"

#include "Virtual Machine/Operations/Operators/Arithmetic.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Types Management/Typecasts.h"


//
// Perform the selected arithmetic operation
//
template<VM::Operations::ArithmeticOpType OpType, class VarType, class RValueType>
VM::RValuePtr VM::Operations::ArithmeticOp<OpType, VarType, RValueType>::ExecuteAndStoreRValue(VM::ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	VarType::BaseStorage ret = 0;

	if(NumParams == 1)
		ret = OperateOnList(stack);
	else if(NumParams == 2)
	{
		if(FirstIsList && !SecondIsList)
		{
			VarType var(stack.GetCurrentTopOfStack());
			VarType::BaseStorage variableval = var.GetValue();
			stack.Pop(VarType::GetStorageSize());
			switch(OpType)
			{
			case Arithmetic_Add:		ret = OperateOnList(stack) + variableval;		break;
			case Arithmetic_Subtract:	ret = OperateOnList(stack) - variableval;		break;
			case Arithmetic_Multiply:	ret = OperateOnList(stack) * variableval;		break;
			case Arithmetic_Divide:		ret = OperateOnList(stack) / variableval;		break;
			default: throw InternalFailureException("Unrecognized arithmetic operation");
			}
		}
		else if(!FirstIsList && SecondIsList)
		{
			ret = OperateOnList(stack);
			VarType var(stack.GetCurrentTopOfStack());
			switch(OpType)
			{
			case Arithmetic_Add:		ret = var.GetValue() + ret;		break;
			case Arithmetic_Subtract:	ret = var.GetValue() - ret;		break;
			case Arithmetic_Multiply:	ret = var.GetValue() * ret;		break;
			case Arithmetic_Divide:		ret = var.GetValue() / ret;		break;
			default: throw InternalFailureException("Unrecognized arithmetic operation");
			}
			stack.Pop(VarType::GetStorageSize());
		}
		else if(FirstIsList && SecondIsList)
		{
			ret = OperateOnList(stack);
			switch(OpType)
			{
			case Arithmetic_Add:		ret = OperateOnList(stack) + ret;		break;
			case Arithmetic_Subtract:	ret = OperateOnList(stack) - ret;		break;
			case Arithmetic_Multiply:	ret = OperateOnList(stack) * ret;		break;
			case Arithmetic_Divide:		ret = OperateOnList(stack) / ret;		break;
			default: throw InternalFailureException("Unrecognized arithmetic operation");
			}
		}
		else
		{
			VarType twovar(stack.GetCurrentTopOfStack());
			VarType onevar(stack.GetOffsetIntoStack(VarType::GetStorageSize()));
			switch(OpType)
			{
			case Arithmetic_Add:		ret = onevar.GetValue() + twovar.GetValue();		break;
			case Arithmetic_Subtract:	ret = onevar.GetValue() - twovar.GetValue();		break;
			case Arithmetic_Multiply:	ret = onevar.GetValue() * twovar.GetValue();		break;
			case Arithmetic_Divide:		ret = onevar.GetValue() / twovar.GetValue();		break;
			default: throw InternalFailureException("Unrecognized arithmetic operation");
			}
			stack.Pop(VarType::GetStorageSize() * 2);
		}
	}
	else
		throw ExecutionException("Invalid set of parameters");

	return RValuePtr(new RValueType(ret));
}

template<VM::Operations::ArithmeticOpType OpType, class VarType, class RValueType>
void VM::Operations::ArithmeticOp<OpType, VarType, RValueType>::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	// Nothing to do here; either the op is called by a PushOperation op, in which case
	// the rvalue execute function will be used; or the op is standalone and has no
	// effects on program state, and therefore can be ignored.
}

//
// Operate on a list of values from the stack
//
template<VM::Operations::ArithmeticOpType OpType, class VarType, class RValueType>
typename VarType::BaseStorage VM::Operations::ArithmeticOp<OpType, VarType, RValueType>::OperateOnList(StackSpace& stack) const
{
	VarType::BaseStorage ret = 0;
	if(OpType == Arithmetic_Multiply || OpType == Arithmetic_Divide)
		ret = 1;

	IntegerVariable typevar(stack.GetCurrentTopOfStack());
	IntegerVariable countvar(stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
	IntegerVariable::BaseStorage type = typevar.GetValue();
	IntegerVariable::BaseStorage count = countvar.GetValue();
	stack.Pop(IntegerVariable::GetStorageSize() * 2);

	if(type != VarType::GetStaticType())
		throw ExecutionException("Type mismatch");

	for(Integer32 i = 0; i < count; ++i)
	{
		VarType var(stack.GetCurrentTopOfStack());
		if(OpType == Arithmetic_Add)
			ret = var.GetValue() + ret;
		else
			ret = var.GetValue() * ret;

		stack.Pop(VarType::GetStorageSize());
	}

	return ret;
}

