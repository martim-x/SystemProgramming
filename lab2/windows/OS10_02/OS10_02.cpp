#include <Windows.h>
#include <stdio.h>
#include "..\OS10_HTAPI\HT.h"

int main()
{
	printf("=== OS10_02: HT API Demo (using OS10_HTAPI static library) ===\n\n");

	const char* filename = "test_ht2.dat";
	int capacity = 100;
	int snapInterval = 5;
	int maxKeyLen = 32;
	int maxPayloadLen = 128;

	// Create HT using library
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