/*++

Module Name:

    MiniFs.c

Abstract:

    This is the main module of the MiniFs miniFilter driver.

Environment:

    Kernel mode

--*/

#include <fltKernel.h>
#include <dontuse.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")


PFLT_FILTER kFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

ULONG gTraceFlags = 0;


#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))

/*************************************************************************
    Prototypes
*************************************************************************/

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
MiniFsInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
MiniFsInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
MiniFsInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
MiniFsUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
MiniFsInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
MiniFsPreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

VOID
MiniFsOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    );

FLT_POSTOP_CALLBACK_STATUS
MiniFsPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

BOOLEAN
MiniFsDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    );

EXTERN_C_END

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, MiniFsUnload)
#pragma alloc_text(PAGE, MiniFsInstanceQueryTeardown)
#pragma alloc_text(PAGE, MiniFsInstanceSetup)
#pragma alloc_text(PAGE, MiniFsInstanceTeardownStart)
#pragma alloc_text(PAGE, MiniFsInstanceTeardownComplete)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_CLOSE,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_READ,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_WRITE,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_SET_INFORMATION,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_QUERY_EA,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_SET_EA,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_SHUTDOWN,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      NULL },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_CLEANUP,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_QUERY_SECURITY,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_SET_SECURITY,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_QUERY_QUOTA,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_SET_QUOTA,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_PNP,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_MDL_READ,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      (PFLT_PRE_OPERATION_CALLBACK)&MiniFsPreOperation,
      (PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation },

    { IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks

    (PFLT_FILTER_UNLOAD_CALLBACK)MiniFsUnload,                                  //  MiniFilterUnload

    (PFLT_INSTANCE_SETUP_CALLBACK)MiniFsInstanceSetup,                           //  InstanceSetup
    (PFLT_INSTANCE_QUERY_TEARDOWN_CALLBACK)MiniFsInstanceQueryTeardown,         //  InstanceQueryTeardown
    (PFLT_INSTANCE_TEARDOWN_CALLBACK)MiniFsInstanceTeardownStart,               //  InstanceTeardownStart
    (PFLT_INSTANCE_TEARDOWN_CALLBACK)MiniFsInstanceTeardownComplete,            //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};



NTSTATUS
MiniFsInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume. This
    gives us a chance to decide if we need to attach to this volume or not.

    If this routine is not defined in the registration structure, automatic
    instances are always created.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("MiniFs!MiniFsInstanceSetup: Entered\n") );

    return STATUS_SUCCESS;
}


NTSTATUS
MiniFsInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request.

    If this routine is not defined in the registration structure, explicit
    detach requests via FltDetachVolume or FilterDetach will always be
    failed.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Returns the status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    return STATUS_SUCCESS;
}


VOID
MiniFsInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the start of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

}


VOID
MiniFsInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the end of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

}


/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Routine can return non success error codes.

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("MiniFs!DriverEntry: Entered\n") );

    //
    //  Register with FltMgr to tell it our callback routines
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &kFilterHandle );

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {

        //
        //  Start filtering i/o
        //

        status = FltStartFiltering( kFilterHandle );

        if (!NT_SUCCESS( status )) {

            FltUnregisterFilter( kFilterHandle );
        }
    }

    return status;
}

NTSTATUS
MiniFsUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unload indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("MiniFs!MiniFsUnload: Entered\n") );

    FltUnregisterFilter( kFilterHandle );

    return STATUS_SUCCESS;
}


/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
MiniFsPreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is a pre-operation dispatch routine for this miniFilter.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS
MiniFsPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine is the post-operation completion routine for this
    miniFilter.

    This is non-pageable because it may be called at DPC level.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-operation routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("MiniFs!(PFLT_POST_OPERATION_CALLBACK)&MiniFsPostOperation: Entered\n") );

    return FLT_POSTOP_FINISHED_PROCESSING;
}

BOOLEAN
MiniFsDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This identifies those operations we want the operation status for.  These
    are typically operations that return STATUS_PENDING as a normal completion
    status.

Arguments:

Return Value:

    TRUE - If we want the operation status
    FALSE - If we don't

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

    //
    //  return boolean state based on which operations we are interested in
    //

    return (BOOLEAN)

            //
            //  Check for oplock operations
            //

             (((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
               ((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK)  ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK)   ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

              ||

              //
              //    Check for directy change notification
              //

              ((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
               (iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
             );
}
