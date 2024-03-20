#ifndef PIXUTILS_H_
#define PIXUTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <winsock.h>
#define MAX_PIXEL_STRING_LENGTH 30
#define DEFAULT_CHANNELS 4
// [STRUCTURES]
/**
 * Structure to represent a color.
 * Each color is represented by its 4 channels: red, green, blue, and alpha.
 */
typedef struct {
    unsigned char r, g, b, a;
} color;

/**
 * Structure to represent an image by its original stb object and its width, height and number of channels..
 */
typedef struct {
    unsigned char *originalImage; // Use original image in order  to retrieve the pixel values to mitigate memory usage
    int width, height, channels;
} image;

/**
 * Structure to represent a chunk of an image.
 * A chunk is represented by its start and end pointers (to the image) , and its x, y coordinates.
 */
typedef struct {
    color *start, *end;
    int x, y;
} chunk;

typedef struct {
    image image;
    chunk chunk;
    SOCKET client;
} processArgs;
// END OF [STRUCTURES]

// [FUNCTION DECLARATIONS]
image loadImage(char* filename);
void resizeImage(image *image, int width, int height, int channels);

void hexifyPixel(int x, int y, color *c, char* buffer);

chunk* makeChunks(image image, int chunk_count);
void processChunk(void* args_);
// END OF [FUNCTION DECLARATIONS]
#endif