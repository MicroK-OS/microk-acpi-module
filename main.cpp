#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <cdefs.h>
#include <mkmi.h>

extern "C" uint32_t VendorID = 0xCAFEBABE;
extern "C" uint32_t ProductID = 0xA3C1C0DE;

#include "acpi/acpi.h"

ACPIManager *acpi;

extern "C" size_t OnInit() {
	acpi = new ACPIManager();

	PrintUserTCB();

	return 0;
}

extern "C" size_t OnExit() {

	return 0;
}
