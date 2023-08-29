#include "acpi.h"
#include "aml_executive.h"
#include "token.h"

#include <mkmi.h>
#include <cdefs.h>

ACPIManager::ACPIManager() : RSDP(NULL), MainSDT(NULL), MainSDTType(0), FADT(NULL), DSDT(NULL), DSDTExecutive(NULL) {
	RSDP = new RSDP2;

	Syscall(SYSCALL_FILE_READ, "ACPI:RSDP", RSDP, sizeof(RSDP2), 0, 0 ,0);
	if(RSDP->Revision != 2) {
		Panic("Invalid ACPI RSDP revision");
	}

	if(!ValidateTable((uint8_t*)RSDP, sizeof(RSDP2))) {
		Panic("Invalid ACPI RSDP checksum");
	}

	if((void*)RSDP->XSDTAddress != NULL) {
		MainSDTType = 8;

		void *addr = RSDP->XSDTAddress + HIGHER_HALF;
		size_t size = ((SDTHeader*)addr)->Length;

		if(ValidateTable((uint8_t*)addr, size)) {
			MainSDT = Malloc(size);
			Memcpy(MainSDT, addr, size);
		} else {
			Panic("Invalid ACPI XSDT checksum");
		}
	} else {
		MainSDTType = 4;

		void *addr = RSDP->RSDTAddress + HIGHER_HALF;
		size_t size = ((SDTHeader*)addr)->Length;

		if(ValidateTable((uint8_t*)addr, size)) {
			MainSDT = Malloc(size);
			Memcpy(MainSDT, addr, size);
		} else {
			Panic("Invalid ACPI RSDT checksum");
		}
	}

	PrintTable(MainSDT);

	int entries = (MainSDT->Length - sizeof(SDTHeader) ) / MainSDTType;
        for (int i = 0; i < entries; i++) {
		/* Getting the table header */
                uintptr_t addr = *(uintptr_t*)((uintptr_t)MainSDT + sizeof(SDTHeader) + (i * MainSDTType));
		SDTHeader *newSDTHeader = addr + HIGHER_HALF;

		if(ValidateTable((uint8_t*)newSDTHeader, newSDTHeader->Length)) {
			PrintTable(newSDTHeader);

			if (Memcmp(newSDTHeader->Signature, "FACP", 4) == 0) {
				FADT = new FADTTable;

				Memcpy(FADT, newSDTHeader, sizeof(FADTTable));

				if(FADT->X_Dsdt != NULL) {
					SDTHeader *dsdt = FADT->X_Dsdt + HIGHER_HALF;
					DSDT = Malloc(dsdt->Length);

					Memcpy(DSDT, dsdt, dsdt->Length);

					PrintTable(DSDT);
				} else if(FADT->Dsdt != NULL) {
					SDTHeader *dsdt = FADT->X_Dsdt + HIGHER_HALF;
					DSDT = Malloc(dsdt->Length);

					Memcpy(DSDT, dsdt, dsdt->Length);

					PrintTable(DSDT);
				} else {
					Panic("No DSDT found");
				}
			} else if (Memcmp(newSDTHeader->Signature, "MCFG", 4) == 0) {
				/* Startup PCI driver */
			} else if (Memcmp(newSDTHeader->Signature, "SSDT", 4) == 0) {
				/* Handle the accessory table */
			} else {
				/* Unknown table */
			}
		} else {
			char sig[5];
			sig[4] = '\0';

			Memcpy(sig, newSDTHeader->Signature, 4);
			MKMI_Printf("Invalid table: %s\r\n", sig);
		}
        }

	if (FADT == NULL)
		Panic("No FADT found");

	if(FADT->SMI_CommandPort == 0 &&
	   (FADT->AcpiEnable == 0 && FADT->AcpiDisable == 0) &&
	   FADT->PM1aControlBlock & 1) {
	   MKMI_Printf("ACPI already enabled.\r\n");
	} else {
		MKMI_Printf("ACPI not yet enabled.\r\n");

		OutPort(FADT->SMI_CommandPort, FADT->AcpiEnable, 8);

		/* TODO: Add delay here (In Linux it's 3 seconds) */
		while((FADT->PM1aControlBlock & 1) == 0);
	}

	DSDTExecutive = new AMLExecutive();
	DSDTExecutive->Parse((uint8_t*)DSDT + sizeof(SDTHeader), DSDT->Length - sizeof(SDTHeader));

	Token *s5 = DSDTExecutive->FindObject("_S5_");
	if (s5 != NULL) {
		MKMI_Printf("Found S5 object.\r\n");
		Token *package = s5->Children->Head;
		if(package != NULL && package->Type == PACKAGE) {
			MKMI_Printf("Found package. %d elements\r\n", package->Package.NumElements);

			Token *data = package->Children->Head;
			for (int i = 0; i <  package->Package.NumElements; i++) {
				if(data->Type == ZERO) MKMI_Printf(" 0\r\n");
				else if(data->Type == INTEGER) MKMI_Printf(" %d\r\n", data->Int.Data);
				else MKMI_Printf("Invalid value type.\r\n");
				
				data = data->Next;
			}
		}


	}

	MKMI_Printf("ACPI initialized.\r\n");
}

bool ACPIManager::ValidateTable(uint8_t *ptr, size_t size) {
	uint8_t checksum = 0;

	for (size_t i = 0; i < size; ++i) checksum += ptr[i];

	if(checksum != 0) return false;

	return true;
}

void ACPIManager::PrintTable(SDTHeader *sdt) {
	char sig[5];
	char oem[7];

	sig[4] = '\0';
	oem[6] = '\0';

	Memcpy(sig, sdt->Signature, 4);
	Memcpy(oem, sdt->OEMID, 6);

	MKMI_Printf("ACPI table found: %s (%s), %d bytes\r\n", sig, oem, sdt->Length);
}

SDTHeader *ACPIManager::FindTable(char *signature, size_t index) {
	static size_t found = 0;

	/* Finding the number of entries */
        int entries = (MainSDT->Length - sizeof(SDTHeader) ) / MainSDTType;
        for (int i = 0; i < entries; i++) {
		/* Getting the table header */
		uintptr_t addr = *(uintptr_t*)((uintptr_t)MainSDT + sizeof(SDTHeader) + (i * MainSDTType));
		SDTHeader *newSDTHeader = addr + HIGHER_HALF;

		/* Checking if the signatures match */
                for (int j = 0; j < 4; j++) {
                        if(newSDTHeader->Signature[j] != signature[j]) break;
			/* If so, then return the table header */
		
			if(j == 3) {
				if(++found < index) break;

				return newSDTHeader;
			}
                }
        }

	/* The request table hasn't been found */
        return NULL;
}

void ACPIManager::Panic(const char *message) {
	MKMI_Printf("ACPI PANIC: %s\r\n", message);
	_exit(128);
}
