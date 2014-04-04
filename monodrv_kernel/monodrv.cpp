#include "stdafx.h"

void monodrvUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS monodrvCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS monodrvDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS monodrvIoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

#ifdef __cplusplus
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath);
#endif

#pragma pack(push, 1)
struct Type7InBuffer {
	unsigned short field0;
	unsigned short field1;
	unsigned short msglen;
	char msg[80];
	unsigned short _unused;
};
#pragma pack(pop)
char* msgbuf[256][256];

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath)
{
	UNICODE_STRING DeviceName,Win32Device;
	PDEVICE_OBJECT DeviceObject = NULL;
	NTSTATUS status;
	unsigned int i,j;
	HANDLE f;
	
	RtlInitUnicodeString(&DeviceName,L"\\Device\\monodrv0");
	RtlInitUnicodeString(&Win32Device,L"\\DosDevices\\Global\\DARKMONO.VXD");
	
	DbgPrintEx(DPFLTR_DEFAULT_ID,DPFLTR_ERROR_LEVEL,"Init, path1 '%wZ' path2 '%wZ'\n",&DeviceName,&Win32Device);
	
	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = monodrvDefaultHandler;
	
	for(i=0;i<256;i++)
		for(j=0;j<256;j++)
			msgbuf[i][j]=NULL;
	
	DriverObject->MajorFunction[IRP_MJ_CREATE] = monodrvCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = monodrvCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]=monodrvIoControl;
	
	DriverObject->DriverUnload = monodrvUnload;
	status = IoCreateDevice(DriverObject,
							0,
							&DeviceName,
							FILE_DEVICE_UNKNOWN,
							0,
							FALSE,
							&DeviceObject);
	
	if (!NT_SUCCESS(status))
		return status;
	if (!DeviceObject)
		return STATUS_UNEXPECTED_IO_ERROR;

	DeviceObject->Flags |= DO_DIRECT_IO;
	DeviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;
	status = IoCreateSymbolicLink(&Win32Device, &DeviceName);

	DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	return STATUS_SUCCESS;
}

void monodrvUnload(IN PDRIVER_OBJECT DriverObject)
{
	unsigned int i=0,j=0,c=0;
	UNICODE_STRING Win32Device;
	RtlInitUnicodeString(&Win32Device,L"\\DosDevices\\Global\\DARKMONO.VXD");
	IoDeleteSymbolicLink(&Win32Device);
	IoDeleteDevice(DriverObject->DeviceObject);
	//free all the strings
	for(i=0;i<256;i++) {
		for(j=0;j<256;j++) {
			if(msgbuf[i][j]!=NULL) {
				c++;
				ExFreePoolWithTag(msgbuf[i][j],L'vrdonom');
			}
		}
	}
	DbgPrintEx(DPFLTR_DEFAULT_ID,DPFLTR_ERROR_LEVEL,"Freed %d string stores\n",c);
}

NTSTATUS monodrvCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	DbgPrintEx(DPFLTR_DEFAULT_ID,DPFLTR_ERROR_LEVEL,"CreateClose called\n");
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS monodrvDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Irp->IoStatus.Status;
}


NTSTATUS monodrvIoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
	NTSTATUS ret=STATUS_SUCCESS;
	PIO_STACK_LOCATION pIoStackIrp=IoGetCurrentIrpStackLocation(Irp);
	unsigned int dwDataWritten=0;
	char* buf=NULL;
	int i=0,j=0,c=0;
	Type7InBuffer* ib=NULL;
	char* retbuf=NULL;
/*	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = dwDataWritten;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	*/
	switch(pIoStackIrp->Parameters.DeviceIoControl.IoControlCode) {
	case 5:
		buf=(char*)Irp->AssociatedIrp.SystemBuffer;
		DbgPrintEx(DPFLTR_DEFAULT_ID,DPFLTR_ERROR_LEVEL,"init, char %x\n",*buf);
	break;
	case 7: //MonoDevice::debugPrint
		ib=(Type7InBuffer*)pIoStackIrp->Parameters.DeviceIoControl.Type3InputBuffer;
		ib->msg[79]=0; //make sure that the last byte is NULL
		i=ib->field0;
		j=ib->field1;
		if(i>255) {
			DbgPrintEx(DPFLTR_DEFAULT_ID,DPFLTR_ERROR_LEVEL,"i beyond 255 (0x%x)\n",i);
			i=255;
		}
		if(j>255) {
			DbgPrintEx(DPFLTR_DEFAULT_ID,DPFLTR_ERROR_LEVEL,"j beyond 255 (0x%x)\n",j);
			j=255;
		}
		if(msgbuf[i][j]==NULL) {
			//todo use something which does not waste tons of precious pool pages! (actually its just 16 or so in use lol)
			msgbuf[i][j]=(char*)ExAllocatePoolWithTag(PagedPool,80,L'vrdonom');
			DbgPrintEx(DPFLTR_DEFAULT_ID,DPFLTR_ERROR_LEVEL,"Allocated new store for %d/%d ('%s')\n",ib->field0,ib->field1,&ib->msg);
		}
		//zero out the memory
		RtlZeroMemory(msgbuf[i][j],80);
		RtlCopyMemory(msgbuf[i][j],&ib->msg,80);
//		DbgPrintEx(DPFLTR_DEFAULT_ID,DPFLTR_ERROR_LEVEL,"Got debug message for level %d/%d: '%s'\n",ib->field0,ib->field1,&ib->msg);
	break;
	case 0x10: //userland: read (use 0x10, not 10 - apparently this switches to buffered io?!?!
		retbuf=(char*)Irp->UserBuffer;
		for(i=0;i<256;i++) {
			for(j=0;j<256;j++) {
				if(msgbuf[i][j]!=NULL) {
					buf=retbuf+(i*256*80)+(j*80);
					//DbgPrintEx(DPFLTR_DEFAULT_ID,DPFLTR_ERROR_LEVEL,"want to copy from %x to %x\n",msgbuf[i][j],buf);
					RtlCopyMemory(buf,msgbuf[i][j],80);
					c++;
				}
			}
		}
		//DbgPrintEx(DPFLTR_DEFAULT_ID,DPFLTR_ERROR_LEVEL,"Copied %d messages to userland\n",c);
	break;
	default:
		DbgPrintEx(DPFLTR_DEFAULT_ID,DPFLTR_ERROR_LEVEL,"Ioctl called, code %x, ibl=%x obl=%x\n",pIoStackIrp->Parameters.DeviceIoControl.IoControlCode,pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength,pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength);
		DbgPrintEx(DPFLTR_DEFAULT_ID,DPFLTR_ERROR_LEVEL,"UserBuffer at %x, SystemBuffer at %x\n",Irp->UserBuffer,Irp->AssociatedIrp.SystemBuffer);
		DbgPrintEx(DPFLTR_DEFAULT_ID,DPFLTR_ERROR_LEVEL,"MdlAddress at %x, Type3InBuffer at %x\n",Irp->MdlAddress,pIoStackIrp->Parameters.DeviceIoControl.Type3InputBuffer);
	break;
	}

	Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Irp->IoStatus.Status;
	return ret;
}
