#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "..\OS10_HTAPI\HT.h"

extern int passed;
extern int failed;

static void Check(const char* name, BOOL condition, const char* detail)
{
	if (condition)
	{
		printf("[PASS] %s\n", name);
		passed++;
	}
	else
	{
		printf("[FAIL] %s: %s\n", name, detail ? detail : "");
		failed++;
	}
}

struct ElementCase
{
	const char* name;
	const char* key;
	int         keylength;
	const char* payload;
	int         payloadlength;
	BOOL        expect;
};

static const char KEY32[]  = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
static const char KEY33[]  = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
static const char PAY128[] = "pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp";
static const char PAY129[] = "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq";

static const ElementCase INSERT_CASES[] =
{
	{ "Insert: обычный ключ",                        "alpha", 5,   "one", 3,        TRUE  },
	{ "Insert: минимальные длины (1/1)",             "x",     1,   "y",   1,        TRUE  },
	{ "Insert: максимальные длины (32/128)",         KEY32,   32,  PAY128, 128,      TRUE  },
	{ "Insert: keylength = 0",                       "abc",   0,   "v",   1,        FALSE },
	{ "Insert: ключ длиннее MaxKeyLength (33)",      KEY33,   33,  "v",   1,        FALSE },
	{ "Insert: payloadlength = 0",                   "k",     1,   "v",   0,        FALSE },
	{ "Insert: значение длиннее MaxPayloadLength",   "k",     1,   PAY129, 129,      FALSE },
	{ "Insert: ключ NULL",                           NULL,    4,   "v",   1,        FALSE },
	{ "Insert: значение NULL",                       "k",     4,   NULL,  3,        FALSE },
};

static const ElementCase GET_CASES[] =
{
	{ "Get: существующий ключ",                      "alpha", 5,   NULL,  0,        TRUE  },
	{ "Get: отсутствующий ключ",                     "nope",  4,   NULL,  0,        FALSE },
	{ "Get: keylength = 0",                          "abc",   0,   NULL,  0,        FALSE },
	{ "Get: ключ NULL",                              NULL,    4,   NULL,  0,        FALSE },
};

static const ElementCase UPDATE_CASES[] =
{
	{ "Update: существующий ключ",                   "alpha", 5,   "two", 3,        TRUE  },
	{ "Update: отсутствующий ключ",                  "nope",  4,   "two", 3,        FALSE },
	{ "Update: значение длиннее MaxPayloadLength",   "alpha", 5,   PAY129, 129,      FALSE },
	{ "Update: payloadlength = 0",                   "alpha", 5,   "v",   0,        FALSE },
	{ "Update: значение NULL",                       "alpha", 5,   NULL,  3,        FALSE },
	{ "Update: keylength = 0",                       "alpha", 0,   "two", 3,        FALSE },
};

static const ElementCase DELETE_CASES[] =
{
	{ "Delete: существующий ключ",                   "beta",  4,   NULL,  0,        TRUE  },
	{ "Delete: отсутствующий ключ",                  "nope",  4,   NULL,  0,        FALSE },
	{ "Delete: keylength = 0",                       "abc",   0,   NULL,  0,        FALSE },
};

static void RunInsertCases(HT::HTHANDLE* ht)
{
	printf("\n=== Parameterized: Insert ===\n");
	for (int i = 0; i < (int)(sizeof(INSERT_CASES) / sizeof(INSERT_CASES[0])); i++)
	{
		const ElementCase* c = &INSERT_CASES[i];
		HT::Element e(c->key, c->keylength, c->payload, c->payloadlength);
		BOOL result = HT::Insert(ht, &e);
		Check(c->name, result == c->expect, c->expect ? "ожидался успех" : "ожидался отказ");
	}
}

static void RunGetCases(HT::HTHANDLE* ht)
{
	printf("\n=== Parameterized: Get ===\n");
	for (int i = 0; i < (int)(sizeof(GET_CASES) / sizeof(GET_CASES[0])); i++)
	{
		const ElementCase* c = &GET_CASES[i];
		HT::Element e(c->key, c->keylength);
		HT::Element* r = HT::Get(ht, &e);
		BOOL found = (r != nullptr);
		Check(c->name, found == c->expect, c->expect ? "ключ не найден" : "ключ найден, хотя не должен");
		if (r)
		{
			free((void*)r->key);
			free((void*)r->payload);
			delete r;
		}
	}
}

static void RunUpdateCases(HT::HTHANDLE* ht)
{
	printf("\n=== Parameterized: Update ===\n");
	for (int i = 0; i < (int)(sizeof(UPDATE_CASES) / sizeof(UPDATE_CASES[0])); i++)
	{
		const ElementCase* c = &UPDATE_CASES[i];
		HT::Element e(c->key, c->keylength);
		BOOL result = HT::Update(ht, &e, c->payload, c->payloadlength);
		Check(c->name, result == c->expect, c->expect ? "ожидался успех" : "ожидался отказ");
	}
}

static void RunDeleteCases(HT::HTHANDLE* ht)
{
	printf("\n=== Parameterized: Delete ===\n");
	for (int i = 0; i < (int)(sizeof(DELETE_CASES) / sizeof(DELETE_CASES[0])); i++)
	{
		const ElementCase* c = &DELETE_CASES[i];
		HT::Element e(c->key, c->keylength);
		BOOL result = HT::Delete(ht, &e);
		Check(c->name, result == c->expect, c->expect ? "ожидался успех" : "ожидался отказ");
	}
}

struct CollisionCase
{
	const char* name;
	const char* key;
	int         keylength;
	const char* payload;
	int         payloadlength;
	BOOL        insertExpect;
	BOOL        getExpect;
};

static const CollisionCase COLLISION_CASES[] =
{
	{ "Коллизия: 'a'  (индекс 3, без пробирования)",  "a",  1, "va",  2, TRUE,  TRUE  },
	{ "Коллизия: 'h'  (индекс 3, слот 4)",            "h",  1, "vh",  2, TRUE,  TRUE  },
	{ "Коллизия: 'k2' (индекс 3, слот 5)",            "k2", 2, "vk2", 3, TRUE,  TRUE  },
};

static void RunCollisionCases(HT::HTHANDLE* ht)
{
	printf("\n=== Parameterized: Collisions (capacity 7) ===\n");

	for (int i = 0; i < (int)(sizeof(COLLISION_CASES) / sizeof(COLLISION_CASES[0])); i++)
	{
		const CollisionCase* c = &COLLISION_CASES[i];
		HT::Element e(c->key, c->keylength, c->payload, c->payloadlength);
		BOOL result = HT::Insert(ht, &e);
		Check(c->name, result == c->insertExpect, "вставка коллизии не удалась");
	}

	for (int i = 0; i < (int)(sizeof(COLLISION_CASES) / sizeof(COLLISION_CASES[0])); i++)
	{
		const CollisionCase* c = &COLLISION_CASES[i];
		HT::Element e(c->key, c->keylength);
		HT::Element* r = HT::Get(ht, &e);
		BOOL ok = (r != nullptr) && r->payloadlength == c->payloadlength &&
		          memcmp(r->payload, c->payload, c->payloadlength) == 0;
		char name[128];
		sprintf_s(name, "Коллизия: чтение '%s' после пробирования", c->key);
		Check(name, ok == c->getExpect, "значение не совпало");
		if (r)
		{
			free((void*)r->key);
			free((void*)r->payload);
			delete r;
		}
	}

	HT::Element eh("h", 1);
	BOOL updated = HT::Update(ht, &eh, "vh_new", 6);
	Check("Коллизия: Update среднего элемента цепочки", updated, "Update не сработал");

	HT::Element* rh = HT::Get(ht, &eh);
	BOOL payloadOk = (rh != nullptr) && rh->payloadlength == 6 && memcmp(rh->payload, "vh_new", 6) == 0;
	Check("Коллизия: значение обновлено на месте", payloadOk, "значение не изменилось");
	if (rh)
	{
		free((void*)rh->key);
		free((void*)rh->payload);
		delete rh;
	}

	HT::Element ek2("k2", 2);
	BOOL deleted = HT::Delete(ht, &ek2);
	Check("Коллизия: удаление последнего в цепочке", deleted, "Delete не сработал");

	HT::Element* rk2 = HT::Get(ht, &ek2);
	Check("Коллизия: удалённый элемент не найден", rk2 == nullptr, "элемент всё ещё доступен");
	if (rk2)
	{
		free((void*)rk2->key);
		free((void*)rk2->payload);
		delete rk2;
	}

	HT::Element* rh2 = HT::Get(ht, &eh);
	Check("Коллизия: сосед по цепочке остался доступен", rh2 != nullptr, "цепочка разорвалась");
	if (rh2)
	{
		free((void*)rh2->key);
		free((void*)rh2->payload);
		delete rh2;
	}
}

static void RunStorageFullCases()
{
	printf("\n=== Parameterized: Storage full (capacity 3) ===\n");

	HT::HTHANDLE* ht = HT::Create(3, 0, 8, 8, (char*)"test_param_full.dat");
	Check("Storage full: создание capacity=3", ht != nullptr, "Create вернул NULL");
	if (!ht)
	{
		return;
	}

	const char* keys[] = { "k1", "k2", "k3", "k4" };
	for (int i = 0; i < 3; i++)
	{
		HT::Element e(keys[i], 2, "v", 1);
		char name[64];
		sprintf_s(name, "Storage full: вставка %s", keys[i]);
		Check(name, HT::Insert(ht, &e), "не удалось заполнить таблицу");
	}

	HT::Element e4(keys[3], 2, "v", 1);
	Check("Storage full: вставка в заполненную таблицу отклонена", !HT::Insert(ht, &e4), "вставка прошла");
	Check("Storage full: сообщение об ошибке", strcmp(HT::GetLastError(ht), "Storage full") == 0, "неверный текст ошибки");

	HT::Close(ht);
}

void RunParameterizedTests()
{
	printf("\n######## Параметризованные тесты ########\n");

	HT::HTHANDLE* ht = HT::Create(100, 0, 32, 128, (char*)"test_param.dat");
	Check("Parameterized: создание хранилища", ht != nullptr, "Create вернул NULL");
	if (!ht)
	{
		return;
	}

	HT::Element seed("beta", 4, "vb", 2);
	HT::Insert(ht, &seed);

	RunInsertCases(ht);
	RunGetCases(ht);
	RunUpdateCases(ht);
	RunDeleteCases(ht);

	HT::Close(ht);

	HT::HTHANDLE* htColl = HT::Create(7, 0, 32, 128, (char*)"test_param_coll.dat");
	Check("Collisions: создание capacity=7", htColl != nullptr, "Create вернул NULL");
	if (htColl)
	{
		RunCollisionCases(htColl);
		HT::Close(htColl);
	}

	RunStorageFullCases();
}
