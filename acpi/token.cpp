#include "token.h"

#include <stdarg.h>
#include <mkmi.h>

TokenList *CreateTokenList() {
	TokenList *list = new TokenList;

	list->TotalNames = 0;
	list->Head = NULL;
	list->Tail = NULL;

	return list;
}

#include <mkmi_log.h>
// Function to create a new token and add it to the token list
void AddToken(TokenList *tokenList, TokenType type, ...) {
	va_list ap;
	va_start(ap, type);

	Token *newToken = new Token;
	newToken->Type = type;
	newToken->Children = NULL;

	newToken->Next = NULL;

	if (tokenList->Head == NULL) {
		tokenList->Head = newToken;
		tokenList->Tail = newToken;
	} else {
		tokenList->Tail->Next = newToken;
		tokenList->Tail = newToken;
	}

	switch(newToken->Type) {
		case ALIAS: {
			NameType *nameOne = va_arg(ap, NameType*);
			NameType *nameTwo = va_arg(ap, NameType*);
			newToken->Alias.NameOne.IsRoot = nameOne->IsRoot;
			newToken->Alias.NameOne.SegmentNumber = nameOne->SegmentNumber;
			newToken->Alias.NameOne.NameSegments = nameOne->NameSegments;
			newToken->Alias.NameTwo.IsRoot = nameTwo->IsRoot;
			newToken->Alias.NameTwo.SegmentNumber = nameTwo->SegmentNumber;
			newToken->Alias.NameTwo.NameSegments = nameTwo->NameSegments;
			}
			break;
		case NAME: {
			NameType *name = va_arg(ap, NameType*);
			TokenList *children = va_arg(ap, TokenList*);
			newToken->Children = children;

			tokenList->Names[tokenList->TotalNames].Token = newToken;
			tokenList->TotalNames++;

			newToken->Name.IsRoot = name->IsRoot;
			newToken->Name.SegmentNumber = name->SegmentNumber;
			newToken->Name.NameSegments = name->NameSegments;
			}
			break;
		case INTEGER: {
			IntegerType *integer = va_arg(ap, IntegerType*);
			newToken->Int.Data = integer->Data;
			newToken->Int.Size = integer->Size;
			}
			break;
		case STRING: {
			const char *str = va_arg(ap, char*);
			size_t len = va_arg(ap, size_t);
			newToken->String = new char[len];
			Memcpy(newToken->String, str, len);
			}
			break;
		case SCOPE: {
			NameType *name = va_arg(ap, NameType*);
			newToken->Scope.PkgLength = va_arg(ap, uint32_t);
			newToken->Scope.Name.IsRoot = name->IsRoot;
			newToken->Scope.Name.SegmentNumber = name->SegmentNumber;
			newToken->Scope.Name.NameSegments = name->NameSegments;
			}
			break;
		case BUFFER: {
			newToken->Buffer.PkgLength = va_arg(ap, uint32_t);
			IntegerType *bufferSize = va_arg(ap, IntegerType*);
			newToken->Buffer.ByteList = va_arg(ap, uint8_t*);
			newToken->Buffer.BufferSize.Data = bufferSize->Data;
			newToken->Buffer.BufferSize.Size = bufferSize->Size;
			}
			break;
		case PACKAGE: {
			uint32_t pkgLength = va_arg(ap, uint32_t);
			uint32_t numElements = va_arg(ap, uint32_t);
			TokenList *children = va_arg(ap, TokenList*);
			newToken->Children = children;

			newToken->Package.PkgLength = pkgLength;
			newToken->Package.NumElements = numElements & 0xFF;
			}
			break;
		case METHOD: {
			uint32_t pkgLength = va_arg(ap, uint32_t);
			NameType *name = va_arg(ap, NameType*);
			uint32_t methodFlags = va_arg(ap, uint32_t);
			newToken->Method.PkgLength = pkgLength;
			newToken->Method.Name.SegmentNumber = name->SegmentNumber;
			newToken->Method.Name.NameSegments = name->NameSegments;
			newToken->Method.Name.IsRoot = name->IsRoot;
			newToken->Method.MethodFlags = methodFlags & 0xFF;
			}
			break;
		case REGION: {
			NameType *name = va_arg(ap, NameType*);
			uint32_t space = va_arg(ap, uint32_t);
			IntegerType *offset = va_arg(ap, IntegerType*);
			IntegerType *len = va_arg(ap, IntegerType*);
			newToken->Region.Name.SegmentNumber = name->SegmentNumber;
			newToken->Region.Name.NameSegments = name->NameSegments;
			newToken->Region.Name.IsRoot = name->IsRoot;
			newToken->Region.RegionSpace = space & 0xFF;
			newToken->Region.RegionOffset.Data = offset->Data;
			newToken->Region.RegionOffset.Size = offset->Size;
			newToken->Region.RegionLen.Data = len->Data;
			newToken->Region.RegionLen.Size = len->Size;
			}
			break;
		case FIELD: {
			uint32_t pkgLength = va_arg(ap, uint32_t);
			NameType *name = va_arg(ap, NameType*);
			uint32_t fieldFlags = va_arg(ap, uint32_t);
			TokenList *children = va_arg(ap, TokenList*);
			newToken->Children = children;

			newToken->Field.PkgLength = pkgLength;
			newToken->Field.Name.SegmentNumber = name->SegmentNumber;
			newToken->Field.Name.NameSegments = name->NameSegments;
			newToken->Field.Name.IsRoot = name->IsRoot;
			newToken->Field.FieldFlags = fieldFlags & 0xFF;
			}
			break;
		case DEVICE: {
			uint32_t pkgLength = va_arg(ap, uint32_t);
			NameType *name = va_arg(ap, NameType*);
			newToken->Device.PkgLength = pkgLength;
			newToken->Device.Name.SegmentNumber = name->SegmentNumber;
			newToken->Device.Name.NameSegments = name->NameSegments;
			newToken->Device.Name.IsRoot = name->IsRoot;
			}
			break;
		case UNKNOWN:
			newToken->UnknownOpcode = va_arg(ap, uint32_t) & 0xFF;
			break;
		default:
			break;
	}

	va_end(ap);
}

// Function to free the memory occupied by the token list
void FreeTokenList(TokenList *tokenList) {
	Token *current = tokenList->Head;
	while (current) {
		if (current->Type == STRING) delete current->String;
		Token *next = current->Next;
		delete current;
		current = next;
	}

	tokenList->Head = NULL;
	tokenList->Tail = NULL;
}
