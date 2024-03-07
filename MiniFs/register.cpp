#include "register.h"

Vector<IrpMjPreFunction>* kPreFuncVector = nullptr;

Vector<IrpMjPostFunction>* kPostFuncVector = nullptr;

Vector<void*>* kDriverFuncVector = nullptr;

void DriverRegister(
	_In_ PDRIVER_OBJECT driver_object,
	_In_ PUNICODE_STRING registry_path
)
{
	kPreFuncVector = new Vector<IrpMjPreFunction>();
	kPostFuncVector = new Vector<IrpMjPostFunction>();
	kDriverFuncVector = new Vector<void*>();
}

void MiniFilterRegister()
{

}

Context* AllocCompletionContext()
{
	Context* context = new Context();
	context->status = new Vector<FLT_PREOP_CALLBACK_STATUS>(kPreFuncVector->Size());

	return context;

}

void DeallocCompletionContext(Context *context)
{

	delete context->status;

	delete context;
}
