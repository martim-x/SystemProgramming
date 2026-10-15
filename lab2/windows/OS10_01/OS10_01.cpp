#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Local HT API implementation (same as in OS10_HTAPI but inline for OS10_01)
namespace HT
{
	struct HTHANDLE
	{
		HTHANDLE() : Capacity(0), SecSnapshotInterval(0), MaxKeyLength(0), MaxPayloadLength(0),
			File(nullptr), FileMapping(nullptr), Addr(nullptr), lastsnaptime(0)
		{
			FileName[0] = '\0';
			LastErrorMessage[0] = '\0';
		}
		int Capacity;
		int SecSnapshotInterval;
		int MaxKeyLength;
		int MaxPayloadLength;
		char FileName[512];
		HANDLE File;
		HANDLE FileMapping;
		LPVOID Addr;
		char LastErrorMessage[512];
		time_t lastsnaptime;
	};

	struct Element
	{
		Element() : key(nullptr), keylength(0), payload(nullptr), payloadlength(0) {}
		Element(const void* k, int kl) : key(k), keylength(kl), payload(nullptr), payloadlength(0) {}
		Element(const void* k, int kl, const void* p, int pl) : key(k), keylength(kl), payload(p), payloadlength(pl) {}
		Element(Element* oldElement, const void* newPayload, int newPayloadLength)
			: key(oldElement->key), keylength(oldElement->keylength), payload(newPayload), payloadlength(newPayloadLength) {}
		const void* key;
		int keylength;
		const void* payload;
		int payloadlength;
	};

	void SetLastError(HTHANDLE* h, const char* msg)
	{
		if (h && msg) strcpy_s(h->LastErrorMessage, msg);
	}

	size_t CalcSize(const HTHANDLE* h)
	{
		size_t headerSize = sizeof(int) * 4 + 512 + sizeof(time_t);
		size_t elemSize = (size_t)h->MaxKeyLength + (size_t)h->MaxPayloadLength + sizeof(int) * 2;
		return headerSize + elemSize * (size_t)h->Capacity;
	}

	size_t ElemOffset(const HTHANDLE* h, int idx)
	{
		size_t headerSize = sizeof(int) * 4 + 512 + sizeof(time_t);
		size_t elemSize = (size_t)h->MaxKeyLength + (size_t)h->MaxPayloadLength + sizeof(int) * 2;
		return headerSize + elemSize * (size_t)idx;
	}

	BOOL IsEmpty(const HTHANDLE* h, int idx)
	{
		size_t off = ElemOffset(h, idx);
		char* p = (char*)h->Addr + off;
		return *(int*)(p + h->MaxKeyLength + h->MaxPayloadLength) == 0;
	}

	int Hash(const void* key, int kl, int cap)
	{
		const unsigned char* b = (const unsigned char*)key;
		unsigned int h = 5381;
		for (int i = 0; i < kl; i++) h = ((h << 5) + h) + b[i];
		return (int)(h % (unsigned int)cap);
	}

	HTHANDLE* Create(int Capacity, int SecSnapshotInterval, int MaxKeyLength, int MaxPayloadLength, const char FileName[512])
	{
		if (Capacity <= 0 || MaxKeyLength <= 0 || MaxPayloadLength <= 0 || !FileName) return nullptr;
		HTHANDLE* h = new HTHANDLE();
		h->Capacity = Capacity; h->SecSnapshotInterval = SecSnapshotInterval;
		h->MaxKeyLength = MaxKeyLength; h->MaxPayloadLength = MaxPayloadLength;
		strcpy_s(h->FileName, FileName);

		h->File = CreateFileA(FileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (h->File == INVALID_HANDLE_VALUE) { SetLastError(h, "CreateFile failed"); delete h; return nullptr; }

		size_t sz = CalcSize(h);
		h->FileMapping = CreateFileMappingA(h->File, NULL, PAGE_READWRITE, (DWORD)(sz >> 32), (DWORD)(sz & 0xFFFFFFFF), NULL);
		if (!h->FileMapping) { SetLastError(h, "CreateFileMapping failed"); CloseHandle(h->File); delete h; return nullptr; }

		h->Addr = MapViewOfFile(h->FileMapping, FILE_MAP_ALL_ACCESS, 0, 0, sz);
		if (!h->Addr) { SetLastError(h, "MapViewOfFile failed"); CloseHandle(h->FileMapping); CloseHandle(h->File); delete h; return nullptr; }

		char* ptr = (char*)h->Addr;
		*(int*)ptr = h->Capacity; ptr += sizeof(int);
		*(int*)ptr = h->SecSnapshotInterval; ptr += sizeof(int);
		*(int*)ptr = h->MaxKeyLength; ptr += sizeof(int);
		*(int*)ptr = h->MaxPayloadLength; ptr += sizeof(int);
		strcpy_s((char*)ptr, 512, h->FileName); ptr += 512;
		*(time_t*)ptr = 0;
		memset(ptr + sizeof(time_t), 0, sz - (ptr - (char*)h->Addr) - sizeof(time_t));
		h->lastsnaptime = 0;
		return h;
	}

	HTHANDLE* Open(const char FileName[512])
	{
		if (!FileName) return nullptr;
		HANDLE f = CreateFileA(FileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (f == INVALID_HANDLE_VALUE) return nullptr;

		char hdr[512 + sizeof(int) * 4 + sizeof(time_t)];
		DWORD br;
		if (!ReadFile(f, hdr, sizeof(hdr), &br, NULL)) { CloseHandle(f); return nullptr; }

		char* p = hdr;
		int Cap = *(int*)p; p += sizeof(int);
		int SSI = *(int*)p; p += sizeof(int);
		int MKL = *(int*)p; p += sizeof(int);
		int MPL = *(int*)p; p += sizeof(int);
		char name[512]; strcpy_s(name, (char*)p); p += 512;
		time_t lst = *(time_t*)p;

		size_t sz = sizeof(int) * 4 + 512 + sizeof(time_t) +
			((size_t)MKL + (size_t)MPL + sizeof(int) * 2) * (size_t)Cap;

		HANDLE fm = CreateFileMappingA(f, NULL, PAGE_READWRITE, 0, 0, NULL);
		if (!fm) { CloseHandle(f); return nullptr; }
		LPVOID a = MapViewOfFile(fm, FILE_MAP_ALL_ACCESS, 0, 0, sz);
		if (!a) { CloseHandle(fm); CloseHandle(f); return nullptr; }

		HTHANDLE* h = new HTHANDLE();
		h->Capacity = Cap; h->SecSnapshotInterval = SSI; h->MaxKeyLength = MKL; h->MaxPayloadLength = MPL;
		strcpy_s(h->FileName, name); h->File = f; h->FileMapping = fm; h->Addr = a; h->lastsnaptime = lst;
		return h;
	}

	BOOL Snap(const HTHANDLE* h)
	{
		if (!h || !h->Addr || !h->FileMapping) return FALSE;
		if (!FlushViewOfFile(h->Addr, 0)) { SetLastError((HTHANDLE*)h, "FlushViewOfFile failed"); return FALSE; }
		if (!FlushFileBuffers(h->File)) { SetLastError((HTHANDLE*)h, "FlushFileBuffers failed"); return FALSE; }
		((HTHANDLE*)h)->lastsnaptime = time(NULL);
		return TRUE;
	}

	BOOL Close(const HTHANDLE* h)
	{
		if (!h) return FALSE;
		BOOL r = TRUE;
		if (!Snap(h)) r = FALSE;
		if (h->Addr) UnmapViewOfFile(h->Addr);
		if (h->FileMapping) CloseHandle(h->FileMapping);
		if (h->File && h->File != INVALID_HANDLE_VALUE) CloseHandle(h->File);
		((HTHANDLE*)h)->Addr = nullptr; ((HTHANDLE*)h)->FileMapping = nullptr; ((HTHANDLE*)h)->File = nullptr;
		delete h;
		return r;
	}

	BOOL Insert(const HTHANDLE* h, const Element* e)
	{
		if (!h || !e || !e->key || e->keylength <= 0 || e->keylength > h->MaxKeyLength ||
			!e->payload || e->payloadlength <= 0 || e->payloadlength > h->MaxPayloadLength)
		{ SetLastError((HTHANDLE*)h, "Invalid Insert params"); return FALSE; }

		int start = Hash(e->key, e->keylength, h->Capacity);
		int idx = start;
		for (int i = 0; i < h->Capacity; i++)
		{
			if (IsEmpty(h, idx))
			{
				size_t off = ElemOffset(h, idx);
				char* d = (char*)h->Addr + off;
				memcpy(d, e->key, e->keylength);
				memset(d + e->keylength, 0, h->MaxKeyLength - e->keylength);
				memcpy(d + h->MaxKeyLength, e->payload, e->payloadlength);
				memset(d + h->MaxKeyLength + e->payloadlength, 0, h->MaxPayloadLength - e->payloadlength);
				*(int*)(d + h->MaxKeyLength + h->MaxPayloadLength) = e->keylength;
				*(int*)(d + h->MaxKeyLength + h->MaxPayloadLength + sizeof(int)) = e->payloadlength;
				return TRUE;
			}
			size_t off = ElemOffset(h, idx);
			char* ex = (char*)h->Addr + off;
			int ekl = *(int*)(ex + h->MaxKeyLength + h->MaxPayloadLength);
			if (ekl == e->keylength && memcmp(ex, e->key, e->keylength) == 0)
			{ SetLastError((HTHANDLE*)h, "Key exists"); return FALSE; }
			idx = (idx + 1) % h->Capacity;
			if (idx == start) break;
		}
		SetLastError((HTHANDLE*)h, "Storage full"); return FALSE;
	}

	BOOL Delete(const HTHANDLE* h, const Element* e)
	{
		if (!h || !e || !e->key || e->keylength <= 0) { SetLastError((HTHANDLE*)h, "Invalid Delete params"); return FALSE; }
		int start = Hash(e->key, e->keylength, h->Capacity);
		int idx = start;
		for (int i = 0; i < h->Capacity; i++)
		{
			if (IsEmpty(h, idx)) { SetLastError((HTHANDLE*)h, "Key not found"); return FALSE; }
			size_t off = ElemOffset(h, idx);
			char* ex = (char*)h->Addr + off;
			int ekl = *(int*)(ex + h->MaxKeyLength + h->MaxPayloadLength);
			if (ekl == e->keylength && memcmp(ex, e->key, e->keylength) == 0)
			{ *(int*)(ex + h->MaxKeyLength + h->MaxPayloadLength) = 0; return TRUE; }
			idx = (idx + 1) % h->Capacity;
			if (idx == start) break;
		}
		SetLastError((HTHANDLE*)h, "Key not found"); return FALSE;
	}

	Element* Get(const HTHANDLE* h, const Element* e)
	{
		if (!h || !e || !e->key || e->keylength <= 0) { SetLastError((HTHANDLE*)h, "Invalid Get params"); return nullptr; }
		int start = Hash(e->key, e->keylength, h->Capacity);
		int idx = start;
		for (int i = 0; i < h->Capacity; i++)
		{
			if (IsEmpty(h, idx)) { SetLastError((HTHANDLE*)h, "Key not found"); return nullptr; }
			size_t off = ElemOffset(h, idx);
			char* ex = (char*)h->Addr + off;
			int ekl = *(int*)(ex + h->MaxKeyLength + h->MaxPayloadLength);
			int epl = *(int*)(ex + h->MaxKeyLength + h->MaxPayloadLength + sizeof(int));
			if (ekl == e->keylength && memcmp(ex, e->key, e->keylength) == 0)
			{
				Element* r = new Element();
				r->key = malloc(ekl); memcpy((void*)r->key, ex, ekl); r->keylength = ekl;
				r->payload = malloc(epl); memcpy((void*)r->payload, ex + h->MaxKeyLength, epl); r->payloadlength = epl;
				return r;
			}
			idx = (idx + 1) % h->Capacity;
			if (idx == start) break;
		}
		SetLastError((HTHANDLE*)h, "Key not found"); return nullptr;
	}

	BOOL Update(const HTHANDLE* h, const Element* oe, const void* np, int npl)
	{
		if (!h || !oe || !oe->key || oe->keylength <= 0 || !np || npl <= 0 || npl > h->MaxPayloadLength)
		{ SetLastError((HTHANDLE*)h, "Invalid Update params"); return FALSE; }
		int start = Hash(oe->key, oe->keylength, h->Capacity);
		int idx = start;
		for (int i = 0; i < h->Capacity; i++)
		{
			if (IsEmpty(h, idx)) { SetLastError((HTHANDLE*)h, "Key not found"); return FALSE; }
			size_t off = ElemOffset(h, idx);
			char* ex = (char*)h->Addr + off;
			int ekl = *(int*)(ex + h->MaxKeyLength + h->MaxPayloadLength);
			if (ekl == oe->keylength && memcmp(ex, oe->key, oe->keylength) == 0)
			{
				memcpy(ex + h->MaxKeyLength, np, npl);
				memset(ex + h->MaxKeyLength + npl, 0, h->MaxPayloadLength - npl);
				*(int*)(ex + h->MaxKeyLength + h->MaxPayloadLength + sizeof(int)) = npl;
				return TRUE;
			}
			idx = (idx + 1) % h->Capacity;
			if (idx == start) break;
		}
		SetLastError((HTHANDLE*)h, "Key not found"); return FALSE;
	}

	char* GetLastError(HTHANDLE* h)
	{
		if (!h || h->LastErrorMessage[0] == '\0') return nullptr;
		return h->LastErrorMessage;
	}

	void print(const Element* e)
	{
		if (!e) { printf("Element: NULL\n"); return; }
		printf("Element: key=");
		if (e->key && e->keylength > 0) for (int i = 0; i < e->keylength; i++) printf("%c", ((char*)e->key)[i]);
		else printf("(null)");
		printf(", keylength=%d", e->keylength);
		if (e->payload && e->payloadlength > 0)
		{
			printf(", payload=");
			for (int i = 0; i < e->payloadlength; i++) printf("%c", ((char*)e->payload)[i]);
			printf(", payloadlength=%d", e->payloadlength);
		}
		else printf(", payload=(null), payloadlength=0");
		printf("\n");
	}
}

int main()
{
	printf("=== OS10_01: HT API Demo (inline implementation) ===\n\n");

	const char* filename = "test_ht1.dat";
	int capacity = 100;
	int snapInterval = 5;
	int maxKeyLen = 32;
	int maxPayloadLen = 128;

	// Create HT
	HT::HTHANDLE* ht = HT::Create(capacity, snapInterval, maxKeyLen, maxPayloadLen, (char*)filename);
	if (!ht)
	{
		printf("Create failed\n");
		return 1;
	}
	printf("Created HT: %s, capacity=%d\n", filename, capacity);

	// Insert elements
	HT::Element e1("key1", 4, "value1", 6);
	HT::Element e2("key2", 4, "value2", 6);
	HT::Element e3("key3", 4, "value3", 6);

	if (HT::Insert(ht, &e1)) printf("Insert key1: OK\n"); else printf("Insert key1: FAIL - %s\n", HT::GetLastError(ht));
	if (HT::Insert(ht, &e2)) printf("Insert key2: OK\n"); else printf("Insert key2: FAIL - %s\n", HT::GetLastError(ht));
	if (HT::Insert(ht, &e3)) printf("Insert key3: OK\n"); else printf("Insert key3: FAIL - %s\n", HT::GetLastError(ht));

	// Get elements
	HT::Element g1("key1", 4);
	HT::Element* r1 = HT::Get(ht, &g1);
	if (r1) { printf("Get key1: "); HT::print(r1); free((void*)r1->key); free((void*)r1->payload); delete r1; }
	else printf("Get key1: FAIL - %s\n", HT::GetLastError(ht));

	HT::Element g2("key2", 4);
	HT::Element* r2 = HT::Get(ht, &g2);
	if (r2) { printf("Get key2: "); HT::print(r2); free((void*)r2->key); free((void*)r2->payload); delete r2; }
	else printf("Get key2: FAIL - %s\n", HT::GetLastError(ht));

	// Update
	if (HT::Update(ht, &g1, "newvalue1", 9)) printf("Update key1: OK\n"); else printf("Update key1: FAIL - %s\n", HT::GetLastError(ht));

	HT::Element* r1u = HT::Get(ht, &g1);
	if (r1u) { printf("Get key1 after update: "); HT::print(r1u); free((void*)r1u->key); free((void*)r1u->payload); delete r1u; }

	// Delete
	if (HT::Delete(ht, &g2)) printf("Delete key2: OK\n"); else printf("Delete key2: FAIL - %s\n", HT::GetLastError(ht));

	HT::Element* r2d = HT::Get(ht, &g2);
	if (!r2d) printf("Get key2 after delete: NOT FOUND (expected)\n");

	// Snap
	if (HT::Snap(ht)) printf("Snap: OK\n"); else printf("Snap: FAIL - %s\n", HT::GetLastError(ht));

	// Close
	if (HT::Close(ht)) printf("Close: OK\n"); else printf("Close: FAIL\n");

	// Open existing
	ht = HT::Open((char*)filename);
	if (ht)
	{
		printf("\nReopened HT: %s\n", filename);
		HT::Element* r3 = HT::Get(ht, &g1);
		if (r3) { printf("Get key1 after reopen: "); HT::print(r3); free((void*)r3->key); free((void*)r3->payload); delete r3; }
		if (HT::Close(ht)) printf("Close: OK\n");
	}
	else
	{
		printf("Open failed\n");
	}

	printf("\n=== Demo complete ===\n");
	return 0;
}