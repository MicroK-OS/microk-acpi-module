#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <cdefs.h>
#include <mkmi.h>
#include <mkmi_log.h>
#include <mkmi_exit.h>
#include <mkmi_memory.h>
#include <mkmi_message.h>
#include <mkmi_syscall.h>

extern "C" uint32_t VendorID = 0xCAFEBABE;
extern "C" uint32_t ProductID = 0xA3C1C0DE;

#include "acpi/acpi.h"

ACPIManager *acpi;

void MessageHandler() {
	MKMI_Message *msg = 0x700000000000;
	uint64_t *data = (uint64_t*)((uintptr_t)msg + 128);

	MKMI_Printf("Message at        0x%x\r\n"
		    " Sender Vendor ID:  %x\r\n"
		    " Sender Product ID: %x\r\n"
		    " Message Size:      %d\r\n"
		    " Data:            0x%x\r\n",
		    msg,
		    msg->SenderVendorID,
		    msg->SenderProductID,
		    msg->MessageSize,
		    *data);

	if (*data == 0x69696969) {
		MKMI_Printf("Asked for MCFG.\r\n");

		uint8_t newMsg[256];
		uint64_t *addr = (uint64_t*)((uintptr_t)newMsg + 128);
		*addr = (uintptr_t)acpi->FindTable("MCFG", 0);
		
		MKMI_Printf("Sending message.\r\n");

		Syscall(SYSCALL_MODULE_MESSAGE_SEND, msg->SenderVendorID, msg->SenderProductID, newMsg, 256, 0 ,0);
	}

	VMFree(data, msg->MessageSize);

	_return(0);
}

extern "C" size_t OnInit() {
	Syscall(SYSCALL_MODULE_MESSAGE_HANDLER, MessageHandler, 0, 0, 0, 0, 0);

	Syscall(SYSCALL_MODULE_SECTION_REGISTER, "ACPI", VendorID, ProductID, 0, 0 ,0);

	acpi = new ACPIManager();

	return 0;
}

extern "C" size_t OnExit() {

	return 0;
}
