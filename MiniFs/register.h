#pragma once

#include <Ntddk.h>
#include <Ntifs.h>
#include <fltKernel.h>

#include "lib/vector/vector.h"

struct IrpMjPreFunction
{
	size_t irp_mj_function_code;
	PFLT_PRE_OPERATION_CALLBACK func;
};

struct IrpMjPostFunction
{
	size_t irp_mj_function_code;
	PFLT_POST_OPERATION_CALLBACK func;
};

extern Vector<IrpMjPreFunction> kPreFuncVector;

extern Vector<IrpMjPostFunction> kPostFuncVector;

extern Vector<void*> kDriverFuncVector;

void DriverRegister();

void MiniFilterRegister();

