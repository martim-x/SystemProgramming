#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "..\OS10_HTAPI\HT.h"

static const char* Err(HT::HTHANDLE* h)
{
	const char* e = HT::GetLastError(h);
	return e ? e : "-";
}

int main()
{
	const char* filename = "test_ht2.dat";
	int capacity = 100;
	int snapInterval = 5;
	int maxKeyLen = 32;
	int maxPayloadLen = 128;

	printf("=== OS10_02: HT API Demo (using OS10_HTAPI static library) ===\n\n");

	HT::HTHANDLE* ht = HT::Create(capacity, snapInterval, maxKeyLen, maxPayloadLen, (char*)filename);
	if (!ht)
	{
		printf("Create failed\n");
		return 1;
	}
	printf("Created HT: %s, capacity=%d, snapshot interval=%d s\n", filename, capacity, snapInterval);

	HT::Element e1("key1", 4, "value1", 6);
	HT::Element e2("key2", 4, "value2", 6);
	HT::Element e3("key3", 4, "value3", 6);

	printf("\n--- Insert ---\n");
	BOOL ok1 = HT::Insert(ht, &e1);
	BOOL ok2 = HT::Insert(ht, &e2);
	BOOL ok3 = HT::Insert(ht, &e3);
	printf("Insert key1: %s\n", ok1 ? "OK" : Err(ht));
	printf("Insert key2: %s\n", ok2 ? "OK" : Err(ht));
	printf("Insert key3: %s\n", ok3 ? "OK" : Err(ht));

	printf("\n--- Get ---\n");
	HT::Element g1("key1", 4);
	HT::Element g2("key2", 4);

	HT::Element* r1 = HT::Get(ht, &g1);
	if (r1)
	{
		printf("Get key1: ");
		HT::print(r1);
		free((void*)r1->key); free((void*)r1->payload); delete r1;
	}
	else
	{
		printf("Get key1: FAIL - %s\n", Err(ht));
	}

	HT::Element* r2 = HT::Get(ht, &g2);
	if (r2)
	{
		printf("Get key2: ");
		HT::print(r2);
		free((void*)r2->key); free((void*)r2->payload); delete r2;
	}
	else
	{
		printf("Get key2: FAIL - %s\n", Err(ht));
	}

	printf("\n--- Update ---\n");
	BOOL okUpd = HT::Update(ht, &g1, "newvalue1", 9);
	printf("Update key1: %s\n", okUpd ? "OK" : Err(ht));

	HT::Element* r1u = HT::Get(ht, &g1);
	if (r1u)
	{
		printf("Get key1 after update: ");
		HT::print(r1u);
		free((void*)r1u->key); free((void*)r1u->payload); delete r1u;
	}

	printf("\n--- Delete ---\n");
	BOOL okDel = HT::Delete(ht, &g2);
	printf("Delete key2: %s\n", okDel ? "OK" : Err(ht));

	HT::Element* r2d = HT::Get(ht, &g2);
	if (r2d)
	{
		printf("Get key2 after delete: STILL EXISTS\n");
		free((void*)r2d->key); free((void*)r2d->payload); delete r2d;
	}
	else
	{
		printf("Get key2 after delete: NOT FOUND (%s)\n", Err(ht));
	}

	printf("\n--- Error handling (GetLastError) ---\n");
	HT::Element dup("key1", 4, "dup", 3);
	BOOL okDup = HT::Insert(ht, &dup);
	printf("Insert duplicate key1: %s\n", okDup ? "OK" : Err(ht));

	HT::Element missing("no_such_key", 11);
	HT::Element* rm = HT::Get(ht, &missing);
	if (rm)
	{
		printf("Get missing key: FOUND\n");
		free((void*)rm->key); free((void*)rm->payload); delete rm;
	}
	else
	{
		printf("Get missing key: NOT FOUND (%s)\n", Err(ht));
	}

	BOOL okDelMissing = HT::Delete(ht, &missing);
	printf("Delete missing key: %s\n", okDelMissing ? "OK" : Err(ht));

	char longKey[40];
	memset(longKey, 'K', sizeof(longKey));
	HT::Element badKey(longKey, 40, "v", 1);
	BOOL okBad = HT::Insert(ht, &badKey);
	printf("Insert key longer than MaxKeyLength (40 > 32): %s\n", okBad ? "OK" : Err(ht));

	printf("\n--- Snap ---\n");
	BOOL okSnap = HT::Snap(ht);
	printf("Snap: %s\n", okSnap ? "OK" : Err(ht));

	BOOL okClose = HT::Close(ht);
	printf("Close: %s\n", okClose ? "OK" : "FAIL");

	printf("\n--- Open existing ---\n");
	ht = HT::Open((char*)filename);
	if (!ht)
	{
		printf("Open failed\n");
		return 1;
	}
	printf("Reopened HT: %s\n", filename);

	HT::Element* r3 = HT::Get(ht, &g1);
	if (r3)
	{
		printf("Get key1 after reopen: ");
		HT::print(r3);
		free((void*)r3->key); free((void*)r3->payload); delete r3;
	}
	else
	{
		printf("Get key1 after reopen: FAIL - %s\n", Err(ht));
	}

	BOOL okClose2 = HT::Close(ht);
	printf("Close: %s\n", okClose2 ? "OK" : "FAIL");

	printf("\n--- Async snapshot (background thread) ---\n");
	HT::HTHANDLE* hta = HT::Create(50, 2, 32, 64, (char*)"test_ht2_async.dat");
	if (!hta)
	{
		printf("Create async HT failed\n");
		return 1;
	}

	HT::Element ea("async_key", 9, "async_value", 11);
	BOOL okA = HT::Insert(hta, &ea);
	printf("Insert async_key: %s\n", okA ? "OK" : Err(hta));

	int before = hta->snapshotsmade;
	printf("Snapshot interval: %d s, snapshots made: %d\n", hta->SecSnapshotInterval, before);
	Sleep(5000);
	int after = hta->snapshotsmade;
	printf("After 5 s with no explicit Snap call: %d snapshot(s)\n", after);
	printf("Async snapshot: %s\n", (after > before) ? "OK - background thread is working" : "FAILED");

	BOOL okCloseA = HT::Close(hta);
	printf("Close async HT: %s\n", okCloseA ? "OK" : "FAIL");

	printf("\n=== Demo complete ===\n");
	return 0;
}
