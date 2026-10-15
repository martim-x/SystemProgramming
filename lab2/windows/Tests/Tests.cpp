#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include "..\OS10_HTAPI\HT.h"

int passed = 0;
int failed = 0;

void AssertTrue(const char* testName, BOOL condition, const char* failMsg = "")
{
	if (condition)
	{
		printf("[PASS] %s\n", testName);
		passed++;
	}
	else
	{
		printf("[FAIL] %s: %s\n", testName, failMsg);
		failed++;
	}
}

void AssertFalse(const char* testName, BOOL condition, const char* failMsg = "")
{
	AssertTrue(testName, !condition, failMsg);
}

void AssertNotNull(const char* testName, void* ptr, const char* failMsg = "")
{
	AssertTrue(testName, ptr != nullptr, failMsg);
}

void AssertNull(const char* testName, void* ptr, const char* failMsg = "")
{
	AssertTrue(testName, ptr == nullptr, failMsg);
}

void TestCreateOpen()
{
	printf("\n=== Test: Create and Open ===\n");
	const char* fname = "test_create.dat";
	HT::HTHANDLE* ht = HT::Create(10, 5, 32, 64, (char*)fname);
	AssertNotNull("Create returns non-null", ht);
	if (ht)
	{
		AssertTrue("Capacity set", ht->Capacity == 10);
		AssertTrue("SnapInterval set", ht->SecSnapshotInterval == 5);
		AssertTrue("MaxKeyLength set", ht->MaxKeyLength == 32);
		AssertTrue("MaxPayloadLength set", ht->MaxPayloadLength == 64);
		AssertTrue("FileName set", strcmp(ht->FileName, fname) == 0);
		AssertTrue("File handle valid", ht->File != nullptr && ht->File != INVALID_HANDLE_VALUE);
		AssertTrue("FileMapping handle valid", ht->FileMapping != nullptr);
		AssertTrue("Addr valid", ht->Addr != nullptr);
		HT::Close(ht);
	}

	// Open existing
	ht = HT::Open((char*)fname);
	AssertNotNull("Open returns non-null", ht);
	if (ht)
	{
		AssertTrue("Capacity preserved", ht->Capacity == 10);
		AssertTrue("FileName preserved", strcmp(ht->FileName, fname) == 0);
		HT::Close(ht);
	}

	// Open non-existent
	ht = HT::Open((char*)"nonexistent.dat");
	AssertNull("Open non-existent returns null", ht);
}

void TestInsertGet()
{
	printf("\n=== Test: Insert and Get ===\n");
	const char* fname = "test_insert.dat";
	HT::HTHANDLE* ht = HT::Create(100, 5, 32, 128, (char*)fname);
	AssertNotNull("Create", ht);
	if (!ht) return;

	// Insert
	HT::Element e1("key1", 4, "value1", 6);
	AssertTrue("Insert key1", HT::Insert(ht, &e1));

	// Get
	HT::Element g1("key1", 4);
	HT::Element* r1 = HT::Get(ht, &g1);
	AssertNotNull("Get key1", r1);
	if (r1)
	{
		AssertTrue("Key length correct", r1->keylength == 4);
		AssertTrue("Payload length correct", r1->payloadlength == 6);
		AssertTrue("Key content correct", memcmp(r1->key, "key1", 4) == 0);
		AssertTrue("Payload content correct", memcmp(r1->payload, "value1", 6) == 0);
		free((void*)r1->key); free((void*)r1->payload); delete r1;
	}

	// Insert duplicate
	HT::Element e1dup("key1", 4, "value1dup", 9);
	AssertFalse("Insert duplicate fails", HT::Insert(ht, &e1dup));

	// Get non-existent
	HT::Element g2("key2", 4);
	HT::Element* r2 = HT::Get(ht, &g2);
	AssertNull("Get non-existent returns null", r2);

	HT::Close(ht);
}

void TestDelete()
{
	printf("\n=== Test: Delete ===\n");
	const char* fname = "test_delete.dat";
	HT::HTHANDLE* ht = HT::Create(100, 5, 32, 128, (char*)fname);
	AssertNotNull("Create", ht);
	if (!ht) return;

	HT::Element e1("key1", 4, "value1", 6);
	HT::Insert(ht, &e1);

	HT::Element g1("key1", 4);
	AssertTrue("Delete existing", HT::Delete(ht, &g1));

	HT::Element* r1 = HT::Get(ht, &g1);
	AssertNull("Get after delete returns null", r1);

	// Delete non-existent
	AssertFalse("Delete non-existent fails", HT::Delete(ht, &g1));

	HT::Close(ht);
}

void TestUpdate()
{
	printf("\n=== Test: Update ===\n");
	const char* fname = "test_update.dat";
	HT::HTHANDLE* ht = HT::Create(100, 5, 32, 128, (char*)fname);
	AssertNotNull("Create", ht);
	if (!ht) return;

	HT::Element e1("key1", 4, "value1", 6);
	HT::Insert(ht, &e1);

	HT::Element g1("key1", 4);
	AssertTrue("Update existing", HT::Update(ht, &g1, "newval", 6));

	HT::Element* r1 = HT::Get(ht, &g1);
	AssertNotNull("Get after update", r1);
	if (r1)
	{
		AssertTrue("Payload updated", r1->payloadlength == 6 && memcmp(r1->payload, "newval", 6) == 0);
		free((void*)r1->key); free((void*)r1->payload); delete r1;
	}

	// Update non-existent
	HT::Element g2("key2", 4);
	AssertFalse("Update non-existent fails", HT::Update(ht, &g2, "val", 3));

	HT::Close(ht);
}

void TestSnap()
{
	printf("\n=== Test: Snap ===\n");
	const char* fname = "test_snap.dat";
	HT::HTHANDLE* ht = HT::Create(100, 5, 32, 128, (char*)fname);
	AssertNotNull("Create", ht);
	if (!ht) return;

	HT::Element e1("key1", 4, "value1", 6);
	HT::Insert(ht, &e1);

	AssertTrue("Snap succeeds", HT::Snap(ht));
	AssertTrue("lastsnaptime updated", ht->lastsnaptime > 0);

	HT::Close(ht);

	// Verify persistence
	ht = HT::Open((char*)fname);
	AssertNotNull("Open after snap", ht);
	if (ht)
	{
		HT::Element g1("key1", 4);
		HT::Element* r1 = HT::Get(ht, &g1);
		AssertNotNull("Data persisted after snap", r1);
		if (r1) { free((void*)r1->key); free((void*)r1->payload); delete r1; }
		HT::Close(ht);
	}
}

void TestCollisionHandling()
{
	printf("\n=== Test: Collision Handling (Open Addressing) ===\n");
	const char* fname = "test_collision.dat";
	// Small capacity to force collisions
	HT::HTHANDLE* ht = HT::Create(5, 5, 32, 128, (char*)fname);
	AssertNotNull("Create small capacity", ht);
	if (!ht) return;

	// Insert multiple elements that may collide
	HT::Element e1("a", 1, "v1", 2);
	HT::Element e2("b", 1, "v2", 2);
	HT::Element e3("c", 1, "v3", 2);
	HT::Element e4("d", 1, "v4", 2);
	HT::Element e5("e", 1, "v5", 2);

	AssertTrue("Insert a", HT::Insert(ht, &e1));
	AssertTrue("Insert b", HT::Insert(ht, &e2));
	AssertTrue("Insert c", HT::Insert(ht, &e3));
	AssertTrue("Insert d", HT::Insert(ht, &e4));
	AssertTrue("Insert e", HT::Insert(ht, &e5));

	// Verify all retrievable
	HT::Element g1("a", 1); HT::Element* r1 = HT::Get(ht, &g1); AssertNotNull("Get a", r1); if (r1) { free((void*)r1->key); free((void*)r1->payload); delete r1; }
	HT::Element g2("b", 1); HT::Element* r2 = HT::Get(ht, &g2); AssertNotNull("Get b", r2); if (r2) { free((void*)r2->key); free((void*)r2->payload); delete r2; }
	HT::Element g3("c", 1); HT::Element* r3 = HT::Get(ht, &g3); AssertNotNull("Get c", r3); if (r3) { free((void*)r3->key); free((void*)r3->payload); delete r3; }
	HT::Element g4("d", 1); HT::Element* r4 = HT::Get(ht, &g4); AssertNotNull("Get d", r4); if (r4) { free((void*)r4->key); free((void*)r4->payload); delete r4; }
	HT::Element g5("e", 1); HT::Element* r5 = HT::Get(ht, &g5); AssertNotNull("Get e", r5); if (r5) { free((void*)r5->key); free((void*)r5->payload); delete r5; }

	// Insert into full - should fail
	HT::Element e6("f", 1, "v6", 2);
	AssertFalse("Insert into full fails", HT::Insert(ht, &e6));

	HT::Close(ht);
}

void TestErrorMessages()
{
	printf("\n=== Test: Error Messages ===\n");
	const char* fname = "test_error.dat";
	HT::HTHANDLE* ht = HT::Create(10, 5, 32, 64, (char*)fname);
	AssertNotNull("Create", ht);
	if (!ht) return;

	// Trigger error by invalid insert
	HT::Element e1(nullptr, 0, "val", 3);
	HT::Insert(ht, &e1);
	AssertNotNull("GetLastError after invalid insert", HT::GetLastError(ht));

	// Trigger error by invalid get
	HT::Element g1(nullptr, 0);
	HT::Get(ht, &g1);
	AssertNotNull("GetLastError after invalid get", HT::GetLastError(ht));

	HT::Close(ht);
}

void TestPersistenceAcrossCloseOpen()
{
	printf("\n=== Test: Persistence Across Close/Open ===\n");
	const char* fname = "test_persist.dat";
	HT::HTHANDLE* ht = HT::Create(100, 5, 32, 128, (char*)fname);
	AssertNotNull("Create", ht);
	if (!ht) return;

	HT::Element e1("persist_key", 11, "persist_value", 13);
	HT::Insert(ht, &e1);
	HT::Snap(ht);
	HT::Close(ht);

	ht = HT::Open((char*)fname);
	AssertNotNull("Open", ht);
	if (ht)
	{
		HT::Element g1("persist_key", 11);
		HT::Element* r1 = HT::Get(ht, &g1);
		AssertNotNull("Data persisted", r1);
		if (r1)
		{
			AssertTrue("Key correct", r1->keylength == 11 && memcmp(r1->key, "persist_key", 11) == 0);
			AssertTrue("Value correct", r1->payloadlength == 13 && memcmp(r1->payload, "persist_value", 13) == 0);
			free((void*)r1->key); free((void*)r1->payload); delete r1;
		}
		HT::Close(ht);
	}
}

void TestMaxSizes()
{
	printf("\n=== Test: Max Key/Payload Sizes ===\n");
	const char* fname = "test_max.dat";
	HT::HTHANDLE* ht = HT::Create(10, 5, 32, 128, (char*)fname);
	AssertNotNull("Create", ht);
	if (!ht) return;

	// Max key length
	char maxKey[33]; memset(maxKey, 'k', 32); maxKey[32] = '\0';
	char maxVal[129]; memset(maxVal, 'v', 128); maxVal[128] = '\0';
	HT::Element e1(maxKey, 32, maxVal, 128);
	AssertTrue("Insert max sizes", HT::Insert(ht, &e1));

	HT::Element g1(maxKey, 32);
	HT::Element* r1 = HT::Get(ht, &g1);
	AssertNotNull("Get max sizes", r1);
	if (r1)
	{
		AssertTrue("Key length 32", r1->keylength == 32);
		AssertTrue("Payload length 128", r1->payloadlength == 128);
		free((void*)r1->key); free((void*)r1->payload); delete r1;
	}

	// Over max key length
	char overKey[34]; memset(overKey, 'k', 33); overKey[33] = '\0';
	HT::Element e2(overKey, 33, "val", 3);
	AssertFalse("Insert over max key fails", HT::Insert(ht, &e2));

	// Over max payload length
	char overVal[130]; memset(overVal, 'v', 129); overVal[129] = '\0';
	HT::Element e3("key", 3, overVal, 129);
	AssertFalse("Insert over max payload fails", HT::Insert(ht, &e3));

	HT::Close(ht);
}

int main()
{
	printf("=== OS10 HT API Test Suite ===\n");

	TestCreateOpen();
	TestInsertGet();
	TestDelete();
	TestUpdate();
	TestSnap();
	TestCollisionHandling();
	TestErrorMessages();
	TestPersistenceAcrossCloseOpen();
	TestMaxSizes();

	printf("\n=== Summary ===\n");
	printf("Passed: %d\n", passed);
	printf("Failed: %d\n", failed);
	printf("Total:  %d\n", passed + failed);

	if (failed == 0)
	{
		printf("\nALL TESTS PASSED!\n");
		return 0;
	}
	else
	{
		printf("\nSOME TESTS FAILED!\n");
		return 1;
	}
}