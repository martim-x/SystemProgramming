#pragma once
#include <Windows.h>

namespace HT
{
	struct HTHANDLE
	{
		HTHANDLE();
		HTHANDLE(int Capacity, int SecSnapshotInterval, int MaxKeyLength, int MaxPayloadLength, const char FileName[512]);
		int     Capacity;
		int     SecSnapshotInterval;
		int     MaxKeyLength;
		int     MaxPayloadLength;
		char    FileName[512];
		HANDLE  File;
		HANDLE  FileMapping;
		LPVOID  Addr;
		char    LastErrorMessage[512];
		time_t  lastsnaptime;
		CRITICAL_SECTION cs;
		HANDLE  SnapThread;
		HANDLE  StopEvent;
		int     snapshotsmade;
	};

	struct Element
	{
		Element();
		Element(const void* key, int keylength);
		Element(const void* key, int keylength, const void* payload, int  payloadlength);
		Element(Element* oldelement, const void* newpayload, int  newpayloadlength);
		const void*     key;
		int             keylength;
		const void*     payload;
		int             payloadlength;
	};

	HTHANDLE*  Create
	(
		int	  Capacity,
		int   SecSnapshotInterval,
		int   MaxKeyLength,
		int   MaxPayloadLength,
		const char  FileName[512]
	);

	HTHANDLE*   Open
	(
		const char    FileName[512]
	);

	BOOL Snap
	(
		const HTHANDLE*    hthandle
	);

	BOOL Close
	(
		const HTHANDLE*    hthandle
	);

	BOOL Insert
	(
		const HTHANDLE* hthandle,
		const Element*  element
	);

	BOOL Delete
	(
		const HTHANDLE* hthandle,
		const Element*  element
	);

	Element* Get
	(
		const HTHANDLE* hthandle,
		const Element*  element
	);

	BOOL Update
	(
		const HTHANDLE* hthandle,
		const Element*  oldelement,
		const void*     newpayload,
		int             newpayloadlength
	);

	char* GetLastError
	(
		HTHANDLE* ht
	);

	void print
	(
		const Element*  element
	);
};
