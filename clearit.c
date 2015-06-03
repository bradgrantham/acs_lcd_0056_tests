#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    HANDLE comHandle;
    int bytesWrittenCount;
    BOOL success;
    DCB dcb;
    char buf[512];
    int row, col, font, type, just;

    if(argc < 2) {
	fprintf(stderr, "usage: %s comfile:\n", argv[0]);
	exit(EXIT_FAILURE);
    }

    comHandle = CreateFile(argv[1], GENERIC_READ | GENERIC_WRITE, 0, NULL,
	    OPEN_EXISTING, 0, NULL );

    if (comHandle == INVALID_HANDLE_VALUE) {
	fprintf(stderr, "failed to CreateFile(%s), %d\n", argv[1], GetLastError());
	exit(EXIT_FAILURE);
    }

    success = GetCommState(comHandle, &dcb);
    if (!success) {
	fprintf(stderr, "GetCommState failed, %d\n", GetLastError());
	exit(EXIT_FAILURE);
    }
    BuildCommDCB("baud=38400 parity=N data=8 stop=1", &dcb );
    success = SetCommState(comHandle, &dcb);
    if (!success) {
	fprintf(stderr, "SetCommState failed, %d\n", GetLastError());
	exit(EXIT_FAILURE);
    }

    sprintf(buf, "%cCFF007F%c", 1, 3);
    success = WriteFile(comHandle, buf, strlen(buf), &bytesWrittenCount, 0);
    if (!success) {
	fprintf(stderr, "WriteFile failed, %d\n", GetLastError());
	exit(EXIT_FAILURE);
    }
    CloseHandle(comHandle);
}
