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

    if(argc < 8) {
	fprintf(stderr, "usage: %s comfile: row col font type just outstring\n", argv[0]);
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

    row = atoi(argv[2]);
    col = atoi(argv[3]);
    font = atoi(argv[4]);
    type = atoi(argv[5]);
    just = atoi(argv[6]);

    sprintf(buf, "%cP%02x%02x%x%x%x%s%c", 1, row, col, font, type, just, argv[7], 3);
    success = WriteFile(comHandle, buf, strlen(buf), &bytesWrittenCount, 0);
    if (!success) {
	fprintf(stderr, "WriteFile failed, %d\n", GetLastError());
	exit(EXIT_FAILURE);
    }
    CloseHandle(comHandle);
}
