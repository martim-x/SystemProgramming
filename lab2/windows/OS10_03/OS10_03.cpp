#include <Windows.h>
#include <stdio.h>
#include "..\OS10_HTAPI\HT.h"

int main()
{
	printf("=== OS10_03: HT API Demo (2 HT instances) ===\n\n");

	const char* filename1 = "test_ht3a.dat";
	const char* filename2 = "test_ht3b.dat";
	int capacity = 50;
	int snapInterval = 5;
	int maxKeyLen = 32;
	int maxPayloadLen = 128;

	// Create first HT
	HT::HTHANDLE* ht1 = HT::Create(capacity, snapInterval, maxKeyLen, maxPayloadLen, (char*)filename1);
	if (!ht1)
	{
		printf("Create HT1 failed\n");
		return 1;
	}
	printf("Created HT1: %s, capacity=%d\n", filename1, capacity);

	// Create second HT
	HT::HTHANDLE* ht2 = HT::Create(capacity, snapInterval, maxKeyLen, maxPayloadLen, (char*)filename2);
	if (!ht2)
	{
		printf("Create HT2 failed\n");
		HT::Close(ht1);
		return 1;
	}
	printf("Created HT2: %s, capacity=%d\n", filename2, capacity);

	// Insert into HT1
	HT::Element e1a("ht1_key1", 8, "ht1_value1", 10);
	HT::Element e1b("ht1_key2", 8, "ht1_value2", 10);
	if (HT::Insert(ht1, &e1a)) printf("HT1 Insert ht1_key1: OK\n"); else printf("HT1 Insert ht1_key1: FAIL - %s\n", HT::GetLastError(ht1));
	if (HT::Insert(ht1, &e1b)) printf("HT1 Insert ht1_key2: OK\n"); else printf("HT1 Insert ht1_key2: FAIL - %s\n", HT::GetLastError(ht1));

	// Insert into HT2
	HT::Element e2a("ht2_key1", 8, "ht2_value1", 10);
	HT::Element e2b("ht2_key2", 8, "ht2_value2", 10);
	if (HT::Insert(ht2, &e2a)) printf("HT2 Insert ht2_key1: OK\n"); else printf("HT2 Insert ht2_key1: FAIL - %s\n", HT::GetLastError(ht2));
	if (HT::Insert(ht2, &e2b)) printf("HT2 Insert ht2_key2: OK\n"); else printf("HT2 Insert ht2_key2: FAIL - %s\n", HT::GetLastError(ht2));

	// Get from HT1
	HT::Element g1a("ht1_key1", 8);
	HT::Element* r1a = HT::Get(ht1, &g1a);
	if (r1a) { printf("HT1 Get ht1_key1: "); HT::print(r1a); free((void*)r1a->key); free((void*)r1a->payload); delete r1a; }
	else printf("HT1 Get ht1_key1: FAIL - %s\n", HT::GetLastError(ht1));

	HT::Element g1b("ht1_key2", 8);
	HT::Element* r1b = HT::Get(ht1, &g1b);
	if (r1b) { printf("HT1 Get ht1_key2: "); HT::print(r1b); free((void*)r1b->key); free((void*)r1b->payload); delete r1b; }
	else printf("HT1 Get ht1_key2: FAIL - %s\n", HT::GetLastError(ht1));

	// Get from HT2
	HT::Element g2a("ht2_key1", 8);
	HT::Element* r2a = HT::Get(ht2, &g2a);
	if (r2a) { printf("HT2 Get ht2_key1: "); HT::print(r2a); free((void*)r2a->key); free((void*)r2a->payload); delete r2a; }
	else printf("HT2 Get ht2_key1: FAIL - %s\n", HT::GetLastError(ht2));

	HT::Element g2b("ht2_key2", 8);
	HT::Element* r2b = HT::Get(ht2, &g2b);
	if (r2b) { printf("HT2 Get ht2_key2: "); HT::print(r2b); free((void*)r2b->key); free((void*)r2b->payload); delete r2b; }
	else printf("HT2 Get ht2_key2: FAIL - %s\n", HT::GetLastError(ht2));

	// Update in HT1
	if (HT::Update(ht1, &g1a, "ht1_newvalue1", 13)) printf("HT1 Update ht1_key1: OK\n"); else printf("HT1 Update ht1_key1: FAIL - %s\n", HT::GetLastError(ht1));
	HT::Element* r1au = HT::Get(ht1, &g1a);
	if (r1au) { printf("HT1 Get ht1_key1 after update: "); HT::print(r1au); free((void*)r1au->key); free((void*)r1au->payload); delete r1au; }

	// Delete in HT2
	if (HT::Delete(ht2, &g2a)) printf("HT2 Delete ht2_key1: OK\n"); else printf("HT2 Delete ht2_key1: FAIL - %s\n", HT::GetLastError(ht2));
	HT::Element* r2ad = HT::Get(ht2, &g2a);
	if (!r2ad) printf("HT2 Get ht2_key1 after delete: NOT FOUND (expected)\n");

	// Snap both
	if (HT::Snap(ht1)) printf("HT1 Snap: OK\n"); else printf("HT1 Snap: FAIL - %s\n", HT::GetLastError(ht1));
	if (HT::Snap(ht2)) printf("HT2 Snap: OK\n"); else printf("HT2 Snap: FAIL - %s\n", HT::GetLastError(ht2));

	// Close both
	if (HT::Close(ht1)) printf("HT1 Close: OK\n"); else printf("HT1 Close: FAIL\n");
	if (HT::Close(ht2)) printf("HT2 Close: OK\n"); else printf("HT2 Close: FAIL\n");

	// Reopen both
	ht1 = HT::Open((char*)filename1);
	ht2 = HT::Open((char*)filename2);
	if (ht1 && ht2)
	{
		printf("\nReopened both HTs\n");
		HT::Element* r1a2 = HT::Get(ht1, &g1a);
		if (r1a2) { printf("HT1 Get ht1_key1 after reopen: "); HT::print(r1a2); free((void*)r1a2->key); free((void*)r1a2->payload); delete r1a2; }
		HT::Element* r2b2 = HT::Get(ht2, &g2b);
		if (r2b2) { printf("HT2 Get ht2_key2 after reopen: "); HT::print(r2b2); free((void*)r2b2->key); free((void*)r2b2->payload); delete r2b2; }
		if (HT::Close(ht1)) printf("HT1 Close: OK\n");
		if (HT::Close(ht2)) printf("HT2 Close: OK\n");
	}
	else
	{
		printf("Reopen failed\n");
	}

	printf("\n=== Demo complete ===\n");
	return 0;
}