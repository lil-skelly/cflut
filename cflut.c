// [SET-UP]
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "client/client.h"
#include <getopt.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "libs/stb_image.h"
#include "libs/stb_image_resize2.h"
#include "libs/log.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define DEFAULT_CHANNELS 4
#define DEFAULT_CHUNKS 4

SOCKET client;
// END OF [SET-UP]

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
// END OF [STRUCTURES]

// [FUNCTION DECLARATIONS]
image loadImage(char* filename);
void resizeImage(image *image, int width, int height, int channels);

char* hexifyPixel(int x, int y, color *c);

chunk* makeChunks(image image, int chunk_count);
void processChunk(image image, chunk chunk, int chunkCount);
// END OF [FUNCTION DECLARATIONS]

// [FUNCTION IMPLEMENTATIONS]
/**
 * Main function of the program.
 * It loads an image, resizes it, divides it into chunks, and processes each chunk.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 */
int main(int argc, char *argv[]) {
    int opt;
    char *image_path = NULL;
    char *dim = NULL;

    // Parse command-line options
    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt) {
            case 'd':
                dim = optarg;
                break;
            default:
                log_error("Usage: %s [-d width:height] <image_path>\n", argv[0]);
                return 1;
        }
    }

    if (optind < argc) {
        image_path = argv[optind];
    } else {
        log_error("Usage: %s [-d width:height] <image_path>\n", argv[0]);
        return 1;
    }

    // Get dimension
    int width, height;
    if (dim != NULL) {
        char *token = strtok(dim, ":");
        if (token != NULL) {
            width = atoi(token);
            token = strtok(NULL, ":");
            if (token != NULL) {
                height = atoi(token);
            } else {
                
                log_error("Invalid dimension format: %s\n", dim);
                return 1;
            }
        } else {
            log_error("Invalid dimension format: %s\n", dim);
            return 1;
        }
    } else {
        log_error("Dimension not provided.\n");
        return 1;
    }

    image imageStruct = loadImage(image_path);
    resizeImage(&imageStruct, width, height, DEFAULT_CHANNELS);
    chunk *imageChunks = makeChunks(imageStruct, DEFAULT_CHUNKS);

    client = initClient();
    int i;
    for (i=0; i < DEFAULT_CHUNKS; i++) {
        log_info("%i\n", i);
        log_info("X:%i Y:%i\n", imageChunks[i].x, imageChunks[i].y);
        processChunk(imageStruct, imageChunks[i], i);
    }

    stbi_image_free(imageStruct.originalImage);
    closesocket(client);
    WSACleanup();
    return 0;

}

/**
 * Load image from file.
 * @param filename Path to file.
 * @return An image struct containing the loaded image.
 */
image loadImage(char* filename) {
    int width, height, channels;
    unsigned char *loadedImage = stbi_load(filename, &width, &height, &channels, 4);
    if (loadedImage == NULL) {
        log_error("Could not load image <%s>\n", filename);
        exit(1);
    }
    log_info("[*] Loaded image <%s>\n", filename, width, height, channels);
    image Image = {loadedImage, width, height, channels};
    return Image;

}

/**
 * Resize an image to the specified dimensions.
 * @param image Pointer to the image to resize.
 * @param width New width of the image.
 * @param height New height of the image.
 * @param channels Number of color channels in the image.
 */
void resizeImage(image *image, int width, int height, int channels) {
    unsigned char* resized_image = (unsigned char*)malloc(width * height * channels * sizeof(unsigned char)); // Allocate the size of the resized image
    stbir_resize_uint8_linear(
        image->originalImage,
        image->width,
        image->height,
        0,
        resized_image,
        width,
        height,
        0,
        channels
    );
    free(image->originalImage); // Free memory of original image
    // Modify original image object.
    image->originalImage = resized_image;
    image->width = width;
    image->height = height;
    image->channels = channels;
    log_info("[*] RESIZED IMAGE -> W:%dpx H:%dpx C:%d\n", image->width, image->height, image->channels);
}

/**
 * Convert a pixel's color to a hexadecimal string.
 * @param x X-coordinate of the pixel.
 * @param y Y-coordinate of the pixel.
 * @param c Color of the pixel.
 * @return A string containing the hexadecimal representation of the pixel's color.
 */
char* hexifyPixel(int x, int y, color *c) {
    char* pixelString = (char*)malloc(50);
    if (pixelString == NULL) {
        log_error("Memory allocation error in hexifyPixel\n");
        exit(1);
    }

    snprintf(
        pixelString,
        50,
        "PX %d %d %02x%02x%02x%02x\n", 
        x, 
        y, 
        c->r, 
        c->g, 
        c->b, 
        c->a
    );

    return pixelString;
}

void processChunk(image image, chunk chunk, int chunkCount) {
    color* colorImage = (color*)image.originalImage;
    int x, y;
    log_info("Start address: %p\nEnd address: %p\n", (void*)chunk.start, (void*)chunk.end);
    for (color* it = chunk.start; it < chunk.end; it++) {
        // send pixel
        int index = it - (color*)image.originalImage;
        x = (index % image.width);
        y = index / image.width;
        char* pixel = hexifyPixel(x, y, it); //offset should be added at the X position
        sendMessage(client, pixel);
        free(pixel);
    }
}

/**
 * Divides an image into the specified number of chunks.
 *
 * This function takes an image and a chunk count as input, and divides the image into that many chunks.
 * Each chunk is represented by a `chunk` struct, which contains a start and end pointer (pointing to the image data),
 * and x, y coordinates representing the top-left corner of the chunk in the image.
 *
 * @param image The image to be divided into chunks.
 * @param chunkCount The number of chunks to divide the image into.
 * @return A pointer to an array of `chunk` structs, each representing a chunk of the image.
 */
chunk* makeChunks(image image, int chunkCount) {
    chunk* chunks = (chunk*)malloc(chunkCount * sizeof(chunk));
    int imageSize = image.width * image.height;
    int chunkSize = image.width / chunkCount;
    log_info("IMAGE SIZE: %d, CHUNK SIZE: %d\n", imageSize, chunkSize);
    int i;
    for (i = 0; i < chunkCount; i++) {
        int start = i * chunkSize;
        int end = start + chunkSize;
        chunks[i] = (chunk){
            .x = 0,
            .y = i * chunkSize,
            .start = (color*)(image.originalImage + start * image.width * DEFAULT_CHANNELS), // removed * image.width
            .end = (color*)(image.originalImage + end * image.width * DEFAULT_CHANNELS),
        };
    }
    return chunks;
}
// END OF [FUNCTION IMPLEMENTATIONS]