#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <unistd.h> */

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

static void skipComments(FILE *fp, char ***comments, size_t *commentCount)
{
    int c;
    char line[512];

    while((c = fgetc(fp)) == '#') {
        fgets(line, sizeof(line) - 1, fp);
	line[strlen(line) - 1] = '\0';
	*comments = realloc(*comments, sizeof(char *) * (*commentCount + 1));
	(*comments)[*commentCount] = strdup(line);
	(*commentCount) ++;
    }
    ungetc(c, fp);
}

int pnmRead(FILE *file, unsigned int *w, unsigned int *h, float **pixels,
    char ***comments, size_t *commentCount)
{
    unsigned char	dummyByte;
    int			i;
    float		max;
    char		token;
    int			width, height;
    float		*rgbPixels;

    *commentCount = 0;
    *comments = NULL;

    fscanf(file, " ");

    skipComments(file, comments, commentCount);

    if(fscanf(file, "P%c ", &token) != 1) {
         fprintf(stderr, "pnmRead: Had trouble reading PNM tag\n");
	 return(0);
    }

    skipComments(file, comments, commentCount);

    if(fscanf(file, "%d ", &width) != 1) {
         fprintf(stderr, "pnmRead: Had trouble reading PNM width\n");
	 return(0);
    }

    skipComments(file, comments, commentCount);

    if(fscanf(file, "%d ", &height) != 1) {
         fprintf(stderr, "pnmRead: Had trouble reading PNM height\n");
	 return(0);
    }

    skipComments(file, comments, commentCount);

    if(token != '1' && token != '4')
        if(fscanf(file, "%f", &max) != 1) {
             fprintf(stderr, "pnmRead: Had trouble reading PNM max value\n");
	     return(0);
        }

    rgbPixels = malloc(width * height * 4 * sizeof(float));
    if(rgbPixels == NULL) {
         fprintf(stderr, "pnmRead: Couldn't allocate %d bytes\n",
	     width * height * 4 * sizeof(float));
         fprintf(stderr, "pnmRead: (For a %d by %d image)\n", width,
	     height);
	 return(0);
    }

    if(token != '4')
	skipComments(file, comments, commentCount);

    if(token != '4')
    fread(&dummyByte, 1, 1, file);	/* chuck white space */

    if(token == '1') {
	for(i = 0; i < width * height; i++) {
	    int pixel;
	    fscanf(file, "%d", &pixel);
	    pixel = 1 - pixel;
	    rgbPixels[i * 4 + 0] = pixel;
	    rgbPixels[i * 4 + 1] = pixel;
	    rgbPixels[i * 4 + 2] = pixel;
	    rgbPixels[i * 4 + 3] = 1.0;
	}
    } else if(token == '2') {
	for(i = 0; i < width * height; i++) {
	    int pixel;
	    fscanf(file, "%d", &pixel);
	    rgbPixels[i * 4 + 0] = pixel / max;
	    rgbPixels[i * 4 + 1] = pixel / max;
	    rgbPixels[i * 4 + 2] = pixel / max;
	    rgbPixels[i * 4 + 3] = 1.0;
	}
    } else if(token == '3') {
	for(i = 0; i < width * height; i++) {
	    int r, g, b;
	    fscanf(file, "%d %d %d", &r, &g, &b);
	    rgbPixels[i * 4 + 0] = r / max;
	    rgbPixels[i * 4 + 1] = g / max;
	    rgbPixels[i * 4 + 2] = b / max;
	    rgbPixels[i * 4 + 3] = 1.0;
	}
    } else if(token == '4') {
        int bitnum = 0;

	for(i = 0; i < width * height; i++) {
	    unsigned char pixel;
	    unsigned char value;

	    if(bitnum == 0)
	        fread(&value, 1, 1, file);

	    pixel = (1 - ((value >> (7 - bitnum)) & 1));
	    rgbPixels[i * 4 + 0] = pixel;
	    rgbPixels[i * 4 + 1] = pixel;
	    rgbPixels[i * 4 + 2] = pixel;
	    rgbPixels[i * 4 + 3] = 1.0;

	    if(++bitnum == 8 || ((i + 1) % width) == 0)
	        bitnum = 0;
	}
    } else if(token == '5') {
	for(i = 0; i < width * height; i++) {
	    unsigned char pixel;
	    fread(&pixel, 1, 1, file);
	    rgbPixels[i * 4 + 0] = pixel / max;
	    rgbPixels[i * 4 + 1] = pixel / max;
	    rgbPixels[i * 4 + 2] = pixel / max;
	    rgbPixels[i * 4 + 3] = 1.0;
	}
    } else if(token == '6') {
	for(i = 0; i < width * height; i++) {
	    unsigned char rgb[3];
	    fread(rgb, 3, 1, file);
	    rgbPixels[i * 4 + 0] = rgb[0] / max;
	    rgbPixels[i * 4 + 1] = rgb[1] / max;
	    rgbPixels[i * 4 + 2] = rgb[2] / max;
	    rgbPixels[i * 4 + 3] = 1.0;
	}
    }
    *w = width;
    *h = height;
    *pixels = rgbPixels;
    return(1);
}

void pnmWrite(FILE *fp, int w, int h, float *pixels)
{
    int i;

    fprintf(fp, "P6 %d %d 255\n", w, h);
    for(i = 0; i < w * h; i++)
    {
        fputc(pixels[i * 4 + 0] * 255, fp);
        fputc(pixels[i * 4 + 1] * 255, fp);
        fputc(pixels[i * 4 + 2] * 255, fp);
    }
}

typedef enum {JUSTIFY_LEFT, JUSTIFY_CENTER, JUSTIFY_RIGHT} justification;

#include "8x16.h"

void drawText(float *imgFloats, int width, int height,
    int x, int y, char *s, float r, float g, float b, justification just)
{
    int    fx, fy, gx, gy;
    u_char    *glyph;
    int    sx, sy;
    char    *e;

    e = s;
    sy = y;
    while(*s != 0) {
        e = s;
        while((*e != '\0') && (*e != '\n'))
            e++;

        if(just == JUSTIFY_RIGHT) sx = x - (e - s) * fontwidth;
        else if(just == JUSTIFY_CENTER) sx = x - (e - s) * fontwidth / 2;
        else sx = x;

        fx = sx;
        fy = sy;
        while(s != e){
            glyph = fontbits + fontheight * *s;
            for(gx = 0; gx < fontwidth; gx++)
                for(gy = 0; gy < fontheight; gy++)
                    if(glyph[gy + gx / 8] & (1 << (7 - (gx % 8)))) {
			int floatIndex;
			int row;
			int col;

			col = fx + gx - (gy / 3);
			row = (fy + gy);
			floatIndex = (col + row * width) * 4;
			imgFloats[floatIndex + 0] = r;
			imgFloats[floatIndex + 1] = g;
			imgFloats[floatIndex + 2] = b;
		    }
            s++;
            fx += fontwidth;
        }

	if(*e == '\n')
	    e++;
        s = e;
        sy = sy + fontheight;
    }
}

float ditherMatrix[2][2] = {
    {.25, .75},
    {.5, 0 },
};

void convertImageToLCD(float *pixels, int w, int h, unsigned char screen[8][128])
{
    int i, j;
    float scale;

    if(w / 128.0 > h / 64.0)
	scale = w / 128.0;
    else
	scale = h / 64.0;

    for(j = 0; j < 64 && j < h / scale; j++) {
	for(i = 0; i < 128 && i < w / scale; i++) {
	    float c;
	    int base;

	    base = ((int)(j * scale) * w + (int)(i * scale)) * 4;

	    c = (pixels[base + 0] + 
		pixels[base + 1] + 
		pixels[base + 2]) / 3.0;
	    if(c <= ditherMatrix[j % 2][i % 2])
		setLCDpixel(screen, i, j);
	    else
		clearLCDpixel(screen, i, j);
	}
    }
}

int main(int argc, char **argv)
{
    unsigned int w, h;
    float *pixels;
    char **comments;
    size_t commentCount;
    unsigned char screen[8][128];
    HANDLE comHandle;
    BOOL success;
    DCB dcb;

    if(argc < 3) {
	fprintf(stderr, "usage: %s comfile: dcbstring\n", argv[0]);
	exit(EXIT_FAILURE);
    }

    if(!pnmRead(stdin, &w, &h, &pixels, &comments, &commentCount)) {
	fprintf(stderr, "Couldn't read image.\n");
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

    memset(screen, 0, 128 * 8);
    convertImageToLCD(pixels, w, h, screen);

    sendLCDScreen(comHandle, screen);

#if 0
    drawText(pixels, w, h, atoi(argv[1]), atoi(argv[2]), argv[3], 0, 0, 0,
	JUSTIFY_LEFT);
    drawText(pixels, w, h, atoi(argv[1]) - 1, atoi(argv[2]) - 2, argv[3],
        .75, .75, .75, JUSTIFY_LEFT);
    pnmWrite(stdout, w, h, pixels);
#endif

    exit(EXIT_SUCCESS);
}
