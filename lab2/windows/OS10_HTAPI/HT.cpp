#include "HT.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace HT
{
	HTHANDLE::HTHANDLE()
		: Capacity(0), SecSnapshotInterval(0), MaxKeyLength(0), MaxPayloadLength(0),
		File(nullptr), FileMapping(nullptr), Addr(nullptr), lastsnaptime(0)
	{
		FileName[0] = '\0';
		LastErrorMessage[0] = '\0';
	}

	HTHANDLE::HTHANDLE(int capacity, int secSnapshotInterval, int maxKeyLength, int maxPayloadLength, const char fileName[512])
		: Capacity(capacity), SecSnapshotInterval(secSnapshotInterval), MaxKeyLength(maxKeyLength), MaxPayloadLength(maxPayloadLength),
		File(nullptr), FileMapping(nullptr), Addr(nullptr), lastsnaptime(0)
	{
		strcpy_s(FileName, fileName);
		LastErrorMessage[0] = '\0';
	}

	Element::Element()
		: key(nullptr), keylength(0), payload(nullptr), payloadlength(0)
	{}

	Element::Element(const void* k, int kl)
		: key(k), keylength(kl), payload(nullptr), payloadlength(0)
	{}

	Element::Element(const void* k, int kl, const void* p, int pl)
		: key(k), keylength(kl), payload(p), payloadlength(pl)
	{}

	Element::Element(Element* oldElement, const void* newPayload, int newPayloadLength)
		: key(oldElement->key), keylength(oldElement->keylength), payload(newPayload), payloadlength(newPayloadLength)
	{}

	void SetLastError(HTHANDLE* h, const char* msg)
	{
		if (h && msg)
		{
			strcpy_s(h->LastErrorMessage, msg);
		}
	}

	size_t CalculateStorageSize(const HTHANDLE* h)
	{
		size_t headerSize = sizeof(int) * 4 + 512 + sizeof(time_t); // Capacity, SecSnapshotInterval, MaxKeyLength, MaxPayloadLength, FileName, lastsnaptime
		size_t elementSize = (size_t)h->MaxKeyLength + (size_t)h->MaxPayloadLength + sizeof(int) * 2; // key, payload, keylength, payloadlength
		return headerSize + elementSize * (size_t)h->Capacity;
	}

	HTHANDLE* Create(int Capacity, int SecSnapshotInterval, int MaxKeyLength, int MaxPayloadLength, const char FileName[512])
	{
		if (Capacity <= 0 || MaxKeyLength <= 0 || MaxPayloadLength <= 0 || !FileName)
		{
			return nullptr;
		}

		HTHANDLE* h = new HTHANDLE(Capacity, SecSnapshotInterval, MaxKeyLength, MaxPayloadLength, FileName);

		h->File = CreateFileA(
			FileName,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (h->File == INVALID_HANDLE_VALUE)
		{
			SetLastError(h, "CreateFile failed");
			delete h;
			return nullptr;
		}

		size_t storageSize = CalculateStorageSize(h);

		h->FileMapping = CreateFileMappingA(
			h->File,
			NULL,
			PAGE_READWRITE,
			(DWORD)(storageSize >> 32),
			(DWORD)(storageSize & 0xFFFFFFFF),
			NULL
		);

		if (!h->FileMapping)
		{
			SetLastError(h, "CreateFileMapping failed");
			CloseHandle(h->File);
			delete h;
			return nullptr;
		}

		h->Addr = MapViewOfFile(h->FileMapping, FILE_MAP_ALL_ACCESS, 0, 0, storageSize);
		if (!h->Addr)
		{
			SetLastError(h, "MapViewOfFile failed");
			CloseHandle(h->FileMapping);
			CloseHandle(h->File);
			delete h;
			return nullptr;
		}

		// Initialize the storage header
		char* ptr = (char*)h->Addr;
		*(int*)ptr = h->Capacity; ptr += sizeof(int);
		*(int*)ptr = h->SecSnapshotInterval; ptr += sizeof(int);
		*(int*)ptr = h->MaxKeyLength; ptr += sizeof(int);
		*(int*)ptr = h->MaxPayloadLength; ptr += sizeof(int);
		strcpy_s((char*)ptr, 512, h->FileName); ptr += 512;
		*(time_t*)ptr = 0; // lastsnaptime

		// Initialize elements area (zero out)
		memset(ptr + sizeof(time_t), 0, storageSize - (ptr - (char*)h->Addr) - sizeof(time_t));

		h->lastsnaptime = 0;
		return h;
	}

	HTHANDLE* Open(const char FileName[512])
	{
		if (!FileName)
		{
			return nullptr;
		}

		HANDLE file = CreateFileA(
			FileName,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (file == INVALID_HANDLE_VALUE)
		{
			return nullptr;
		}

		// Read header to get storage size
		char header[512 + sizeof(int) * 4 + sizeof(time_t)];
		DWORD bytesRead;
		if (!ReadFile(file, header, sizeof(header), &bytesRead, NULL))
		{
			CloseHandle(file);
			return nullptr;
		}

		char* ptr = header;
		int Capacity = *(int*)ptr; ptr += sizeof(int);
		int SecSnapshotInterval = *(int*)ptr; ptr += sizeof(int);
		int MaxKeyLength = *(int*)ptr; ptr += sizeof(int);
		int MaxPayloadLength = *(int*)ptr; ptr += sizeof(int);
		char name[512];
		strcpy_s(name, (char*)ptr); ptr += 512;
		time_t lastsnaptime = *(time_t*)ptr;

		size_t storageSize = sizeof(int) * 4 + 512 + sizeof(time_t) + 
			((size_t)MaxKeyLength + (size_t)MaxPayloadLength + sizeof(int) * 2) * (size_t)Capacity;

		HANDLE mapping = CreateFileMappingA(file, NULL, PAGE_READWRITE, 0, 0, NULL);
		if (!mapping)
		{
			CloseHandle(file);
			return nullptr;
		}

		LPVOID addr = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, storageSize);
		if (!addr)
		{
			CloseHandle(mapping);
			CloseHandle(file);
			return nullptr;
		}

		HTHANDLE* h = new HTHANDLE(Capacity, SecSnapshotInterval, MaxKeyLength, MaxPayloadLength, name);
		h->File = file;
		h->FileMapping = mapping;
		h->Addr = addr;
		h->lastsnaptime = lastsnaptime;
		return h;
	}

	BOOL Snap(const HTHANDLE* h)
	{
		if (!h || !h->Addr || !h->FileMapping)
		{
			return FALSE;
		}

		if (!FlushViewOfFile(h->Addr, 0))
		{
			SetLastError((HTHANDLE*)h, "FlushViewOfFile failed");
			return FALSE;
		}

		if (!FlushFileBuffers(h->File))
		{
			SetLastError((HTHANDLE*)h, "FlushFileBuffers failed");
			return FALSE;
		}

		((HTHANDLE*)h)->lastsnaptime = time(NULL);
		return TRUE;
	}

	BOOL Close(const HTHANDLE* h)
	{
		if (!h)
		{
			return FALSE;
		}

		BOOL result = TRUE;
		if (!Snap(h))
		{
			result = FALSE;
		}

		if (h->Addr)
		{
			UnmapViewOfFile(h->Addr);
		}
		if (h->FileMapping)
		{
			CloseHandle(h->FileMapping);
		}
		if (h->File && h->File != INVALID_HANDLE_VALUE)
		{
			CloseHandle(h->File);
		}

		((HTHANDLE*)h)->Addr = nullptr;
		((HTHANDLE*)h)->FileMapping = nullptr;
		((HTHANDLE*)h)->File = nullptr;

		delete h;
		return result;
	}

	size_t GetElementOffset(const HTHANDLE* h, int index)
	{
		size_t headerSize = sizeof(int) * 4 + 512 + sizeof(time_t);
		size_t elementSize = (size_t)h->MaxKeyLength + (size_t)h->MaxPayloadLength + sizeof(int) * 2;
		return headerSize + elementSize * (size_t)index;
	}

	BOOL IsSlotEmpty(const HTHANDLE* h, int index)
	{
		size_t offset = GetElementOffset(h, index);
		char* elementPtr = (char*)h->Addr + offset;
		int keylength = *(int*)(elementPtr + h->MaxKeyLength + h->MaxPayloadLength);
		return keylength == 0;
	}

	int HashFunction(const void* key, int keylength, int capacity)
	{
		const unsigned char* bytes = (const unsigned char*)key;
		unsigned int hash = 5381;
		for (int i = 0; i < keylength; i++)
		{
			hash = ((hash << 5) + hash) + bytes[i]; // hash * 33 + c
		}
		return (int)(hash % (unsigned int)capacity);
	}

	BOOL Insert(const HTHANDLE* h, const Element* element)
	{
		if (!h || !element || !element->key || element->keylength <= 0 || element->keylength > h->MaxKeyLength ||
			!element->payload || element->payloadlength <= 0 || element->payloadlength > h->MaxPayloadLength)
		{
			SetLastError((HTHANDLE*)h, "Invalid parameters for Insert");
			return FALSE;
		}

		int startIndex = HashFunction(element->key, element->keylength, h->Capacity);
		int index = startIndex;

		for (int i = 0; i < h->Capacity; i++)
		{
			if (IsSlotEmpty(h, index))
			{
				size_t offset = GetElementOffset(h, index);
				char* dest = (char*)h->Addr + offset;

				memcpy(dest, element->key, element->keylength);
				memset(dest + element->keylength, 0, h->MaxKeyLength - element->keylength);

				memcpy(dest + h->MaxKeyLength, element->payload, element->payloadlength);
				memset(dest + h->MaxKeyLength + element->payloadlength, 0, h->MaxPayloadLength - element->payloadlength);

				*(int*)(dest + h->MaxKeyLength + h->MaxPayloadLength) = element->keylength;
				*(int*)(dest + h->MaxKeyLength + h->MaxPayloadLength + sizeof(int)) = element->payloadlength;

				return TRUE;
			}

			// Check if key already exists
			size_t offset = GetElementOffset(h, index);
			char* existing = (char*)h->Addr + offset;
			int existingKeyLength = *(int*)(existing + h->MaxKeyLength + h->MaxPayloadLength);
			if (existingKeyLength == element->keylength && memcmp(existing, element->key, element->keylength) == 0)
			{
				SetLastError((HTHANDLE*)h, "Key already exists");
				return FALSE;
			}

			index = (index + 1) % h->Capacity;
			if (index == startIndex)
			{
				break;
			}
		}

		SetLastError((HTHANDLE*)h, "Storage full");
		return FALSE;
	}

	BOOL Delete(const HTHANDLE* h, const Element* element)
	{
		if (!h || !element || !element->key || element->keylength <= 0)
		{
			SetLastError((HTHANDLE*)h, "Invalid parameters for Delete");
			return FALSE;
		}

		int startIndex = HashFunction(element->key, element->keylength, h->Capacity);
		int index = startIndex;

		for (int i = 0; i < h->Capacity; i++)
		{
			if (IsSlotEmpty(h, index))
			{
				SetLastError((HTHANDLE*)h, "Key not found");
				return FALSE;
			}

			size_t offset = GetElementOffset(h, index);
			char* existing = (char*)h->Addr + offset;
			int existingKeyLength = *(int*)(existing + h->MaxKeyLength + h->MaxPayloadLength);

			if (existingKeyLength == element->keylength && memcmp(existing, element->key, element->keylength) == 0)
			{
				// Mark as empty by setting keylength to 0
				*(int*)(existing + h->MaxKeyLength + h->MaxPayloadLength) = 0;
				return TRUE;
			}

			index = (index + 1) % h->Capacity;
			if (index == startIndex)
			{
				break;
			}
		}

		SetLastError((HTHANDLE*)h, "Key not found");
		return FALSE;
	}

	Element* Get(const HTHANDLE* h, const Element* element)
	{
		if (!h || !element || !element->key || element->keylength <= 0)
		{
			SetLastError((HTHANDLE*)h, "Invalid parameters for Get");
			return nullptr;
		}

		int startIndex = HashFunction(element->key, element->keylength, h->Capacity);
		int index = startIndex;

		for (int i = 0; i < h->Capacity; i++)
		{
			if (IsSlotEmpty(h, index))
			{
				SetLastError((HTHANDLE*)h, "Key not found");
				return nullptr;
			}

			size_t offset = GetElementOffset(h, index);
			char* existing = (char*)h->Addr + offset;
			int existingKeyLength = *(int*)(existing + h->MaxKeyLength + h->MaxPayloadLength);
			int existingPayloadLength = *(int*)(existing + h->MaxKeyLength + h->MaxPayloadLength + sizeof(int));

			if (existingKeyLength == element->keylength && memcmp(existing, element->key, element->keylength) == 0)
			{
				Element* result = new Element();
				result->key = malloc(existingKeyLength);
				memcpy((void*)result->key, existing, existingKeyLength);
				result->keylength = existingKeyLength;

				result->payload = malloc(existingPayloadLength);
				memcpy((void*)result->payload, existing + h->MaxKeyLength, existingPayloadLength);
				result->payloadlength = existingPayloadLength;

				return result;
			}

			index = (index + 1) % h->Capacity;
			if (index == startIndex)
			{
				break;
			}
		}

		SetLastError((HTHANDLE*)h, "Key not found");
		return nullptr;
	}

	BOOL Update(const HTHANDLE* h, const Element* oldElement, const void* newPayload, int newPayloadLength)
	{
		if (!h || !oldElement || !oldElement->key || oldElement->keylength <= 0 ||
			!newPayload || newPayloadLength <= 0 || newPayloadLength > h->MaxPayloadLength)
		{
			SetLastError((HTHANDLE*)h, "Invalid parameters for Update");
			return FALSE;
		}

		int startIndex = HashFunction(oldElement->key, oldElement->keylength, h->Capacity);
		int index = startIndex;

		for (int i = 0; i < h->Capacity; i++)
		{
			if (IsSlotEmpty(h, index))
			{
				SetLastError((HTHANDLE*)h, "Key not found");
				return FALSE;
			}

			size_t offset = GetElementOffset(h, index);
			char* existing = (char*)h->Addr + offset;
			int existingKeyLength = *(int*)(existing + h->MaxKeyLength + h->MaxPayloadLength);

			if (existingKeyLength == oldElement->keylength && memcmp(existing, oldElement->key, oldElement->keylength) == 0)
			{
				memcpy(existing + h->MaxKeyLength, newPayload, newPayloadLength);
				memset(existing + h->MaxKeyLength + newPayloadLength, 0, h->MaxPayloadLength - newPayloadLength);
				*(int*)(existing + h->MaxKeyLength + h->MaxPayloadLength + sizeof(int)) = newPayloadLength;
				return TRUE;
			}

			index = (index + 1) % h->Capacity;
			if (index == startIndex)
			{
				break;
			}
		}

		SetLastError((HTHANDLE*)h, "Key not found");
		return FALSE;
	}

	char* GetLastError(HTHANDLE* ht)
	{
		if (!ht || ht->LastErrorMessage[0] == '\0')
		{
			return nullptr;
		}
		return ht->LastErrorMessage;
	}

	void print(const Element* element)
	{
		if (!element)
		{
			printf("Element: NULL\n");
			return;
		}

		printf("Element: key=");
		if (element->key && element->keylength > 0)
		{
			for (int i = 0; i < element->keylength; i++)
			{
				printf("%c", ((char*)element->key)[i]);
			}
		}
		else
		{
			printf("(null)");
		}

		printf(", keylength=%d", element->keylength);

		if (element->payload && element->payloadlength > 0)
		{
			printf(", payload=");
			for (int i = 0; i < element->payloadlength; i++)
			{
				printf("%c", ((char*)element->payload)[i]);
			}
			printf(", payloadlength=%d", element->payloadlength);
		}
		else
		{
			printf(", payload=(null), payloadlength=0");
		}
		printf("\n");
	}
}