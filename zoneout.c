#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define BAND_SIZE 20
#define SPEED 1.333

static char int_to_hex[] = "0123456789ABCDEF";

void setLCDpixel(unsigned char screen[8][128], int x, int y)
{
    screen[y / 8][x] |= 1 << (y % 8);
}

void clearLCDpixel(unsigned char screen[8][128], int x, int y)
{
    screen[y / 8][x] &= ~(1 << (y % 8));
}

void sendLCDScreen(HANDLE comHandle, unsigned char screen[8][128])
{
    int y, x;
    char buf[512];
    BOOL success;
    int bytesOutCount;

    for(y = 0; y < 8; y++) {
	sprintf(buf, "%cH%02x0080", 1, 1 << y);
	success = WriteFile(comHandle, buf, strlen(buf), &bytesOutCount, 0);
	if (!success) {
	    fprintf(stderr, "WriteFile failed, %d\n", GetLastError());
	    exit(EXIT_FAILURE);
	}

	for(x = 0; x < 128; x++) {
	    sprintf(buf, "%02X", screen[y][x]);
	    success = WriteFile(comHandle, buf, strlen(buf), &bytesOutCount, 0);
	    if (!success) {
		fprintf(stderr, "WriteFile failed, %d\n", GetLastError());
		exit(EXIT_FAILURE);
	    }
	}

	sprintf(buf, "%c", 3);
	success = WriteFile(comHandle, buf, strlen(buf), &bytesOutCount, 0);
	if (!success) {
	    fprintf(stderr, "WriteFile failed, %d\n", GetLastError());
	    exit(EXIT_FAILURE);
	}
    }
}

void sendDiffLCDScreen(HANDLE comHandle, unsigned char old_screen[8][128],
	unsigned char screen[8][128])
{
    int y, x;
    char buf[512];
    BOOL success;
    int bytesOutCount;
    int len;
    int i;
    int total = 0;

    for(y = 0; y < 8; y++) {
	x = 0;
	while (x < 128) {
	    if (old_screen[y][x] == screen[y][x]) {
		x++;
		continue;
	    }
	    len = 4;
	    if (x + len > 128) {
		len = 128 - x;
	    }
	    while (x + len < 128 &&
		    old_screen[y][x + len] != screen[y][x + len]) {

		len++;
	    }

	    sprintf(buf, "%cH%02X%02X%02X", 1, 1 << y, x, len);
	    success = WriteFile(comHandle, buf, strlen(buf), &bytesOutCount, 0);
	    if (!success) {
		fprintf(stderr, "WriteFile failed, %d\n", GetLastError());
		exit(EXIT_FAILURE);
	    }

	    for(i = 0; i < len; i++) {
		buf[i*2] = int_to_hex[screen[y][x + i] / 16];
		buf[i*2 + 1] = int_to_hex[screen[y][x + i] % 16];
		total++;
	    }
	    buf[len*2] = 0;

	    success = WriteFile(comHandle, buf, strlen(buf), &bytesOutCount, 0);
	    if (!success) {
		fprintf(stderr, "WriteFile failed, %d\n", GetLastError());
		exit(EXIT_FAILURE);
	    }

	    sprintf(buf, "%c", 3);
	    success = WriteFile(comHandle, buf, strlen(buf), &bytesOutCount, 0);
	    if (!success) {
		fprintf(stderr, "WriteFile failed, %d\n", GetLastError());
		exit(EXIT_FAILURE);
	    }

	    x += len;
	}

    }
}


void zoneout(HANDLE comHandle)
{
    char buf[512];
    int starttime;
    int bytesOutCount;
    BOOL success;
    unsigned char screen[8][128];
    unsigned char old_screen[8][128];
    static int firstTime = 1;
    static float dist[64][128];
    static float angle[64][128];
    int i, x, y;
    float p;

    starttime = time(NULL);
    p = 0;
    while(time(NULL) - starttime < 3600) {

	memcpy(old_screen, screen, sizeof(screen));

	for(y = 0; y < 64; y++)
	    for(x = 0; x < 128; x++) {
		float u, v;
		float band;
		if(firstTime) {
		    u = x - 64;
		    v = y - 32;
		    dist[y][x] = sqrt(u * u + v * v);
		    angle[y][x] = atan2(u, v) / (3.14159 * 2);
		}
		if(fmod(dist[y][x] + angle[y][x] * 2 * BAND_SIZE + 100000 + p, 2 * BAND_SIZE) < BAND_SIZE)
		    setLCDpixel(screen, x, y);
		else
		    clearLCDpixel(screen, x, y);
	    }

	if(firstTime)
	    sendLCDScreen(comHandle, screen);
	else
	    sendDiffLCDScreen(comHandle, old_screen, screen);

	firstTime = 0;
	p -= SPEED;
    }
}

int main(int argc, char **argv)
{
    HANDLE comHandle;
    BOOL success;
    DCB dcb;

    if(argc < 3) {
	fprintf(stderr, "usage: %s comfile: dcbstring\n", argv[0]);
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
    BuildCommDCB(argv[2], &dcb );
    success = SetCommState(comHandle, &dcb);
    if (!success) {
	fprintf(stderr, "SetCommState failed, %d\n", GetLastError());
	exit(EXIT_FAILURE);
    }

    zoneout(comHandle);

    CloseHandle(comHandle);
}
