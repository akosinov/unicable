#include <Windows.h>
#include <stdio.h>

#define MAX_NUM_MPUSB_DEV           127
#define MPUSB_FAIL                  0
#define MPUSB_SUCCESS               1

#define MP_WRITE                    0
#define MP_READ                     1

void log(char* fmt, ...) {
	va_list ptr;
	FILE *logfile = fopen("d:\\dlllog.log", "a+t");
	va_start(ptr, fmt);
	vfprintf(logfile, fmt, ptr);
	fprintf(logfile, "\n");
	va_end(ptr);
	fclose(logfile);
}

extern "C" __declspec(dllexport)
DWORD _MPUSBGetDLLVersion(void) {
	log("_GetDllBersion");
	return 0;
}

char *pidvid = "vid_04d8&pid_000c";

extern "C" __declspec(dllexport)
DWORD _MPUSBGetDeviceCount(PCHAR pVID_PID) {
	if (!strcmp(pVID_PID,pidvid)) {
		return 1;
	}
	return 0;
}

int cur_handle = 1;

extern "C" __declspec(dllexport)
HANDLE _MPUSBOpen(DWORD instance,    // Input
PCHAR pVID_PID,    // Input
PCHAR pEP,         // Input
DWORD dwDir,       // Input
DWORD dwReserved) { // Input <Future Use>
	if (!strcmp(pVID_PID, pidvid)) {
		return (HANDLE)cur_handle++;
	}
	return INVALID_HANDLE_VALUE;
}

extern "C" __declspec(dllexport)
DWORD _MPUSBRead(HANDLE handle,              // Input
PVOID pData,                // Output
DWORD dwLen,                // Input
PDWORD pLength,             // Output
DWORD dwMilliseconds) {      // Input
	log("read HANDLE=%d len=%d put 0x01", handle,dwLen);
	for (DWORD i = 0; i < dwLen; i++)
		((UCHAR*)pData)[i] = 0x01;
	*pLength = dwLen;
	return MPUSB_SUCCESS;
}

extern "C" __declspec(dllexport)
DWORD _MPUSBWrite(HANDLE handle,             // Input
PVOID pData,               // Input
DWORD dwLen,               // Input
PDWORD pLength,            // Output
DWORD dwMilliseconds){     // Input
	log("write HANDLE=%d", handle);
	for (DWORD i = 0; i < dwLen; i++)
		log("%02X",((UCHAR*)pData)[i]);
	*pLength = dwLen;
	return MPUSB_SUCCESS;
}

extern "C" __declspec(dllexport)
DWORD _MPUSBReadInt(HANDLE handle,           // Input
PVOID pData,             // Output
DWORD dwLen,             // Input
PDWORD pLength,          // Output
DWORD dwMilliseconds){   // Input
	log("readint HANDLE=%d", handle);
	return 0;
}

extern "C" __declspec(dllexport)
BOOL _MPUSBClose(HANDLE handle) {
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		log("process attach");
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		log("process detach");
		break;
	}
	return TRUE;
}

