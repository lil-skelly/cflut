#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "pixutils.h"
#include "../client/client.h"
#include "../log/log.h"

#include "../stb_image/stb_image.h"
#include "../stb_image/stb_image_resize2.h"

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
    int chunkSize = image.width / chunkCount;
    log_info("[*] CHUNK SIZE: %d\n", chunkSize);
    int i;
    for (i = 0; i < chunkCount; i++) {
        int start = i * chunkSize;
        int end = start + chunkSize;
        chunks[i] = (chunk){
            .x = 0,
            .y = i * chunkSize,
            .start = (color*)(image.originalImage + start * image.width * DEFAULT_CHANNELS),
            .end = (color*)(image.originalImage + end * image.width * DEFAULT_CHANNELS),
        };
    }
    return chunks;
}

void processChunk(void* args_) {
    // Unpack arguments
    processArgs* args = (processArgs*)args_;
    image image = args->image;
    chunk chunk = args->chunk;
    SOCKET client = args->client;

    color* colorImage = (color*)image.originalImage;
    int x, y;

    char pixelBuffer[MAX_PIXEL_STRING_LENGTH * 100]; // buffer for multiple pixels
    int bufferIndex = 0;
    int maxBufIndex = sizeof(pixelBuffer) - MAX_PIXEL_STRING_LENGTH;
    for (color* it = chunk.start; it < chunk.end; it++) {
        int index = it - (color*)image.originalImage;
        x = (index % image.width);
        y = index / image.width;
        bufferIndex += snprintf(
            pixelBuffer,
            MAX_PIXEL_STRING_LENGTH,
            "PX %d %d %02x%02x%02x%02x\n", 
            x, 
            y, 
            it->r, 
            it->g, 
            it->b, 
            it->a
        );

        if (bufferIndex >= maxBufIndex) {
            sendMessage(client, pixelBuffer);
            bufferIndex = 0;
        }

        if (bufferIndex > 0) {
            sendMessage(client, pixelBuffer);
        }
    }
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
        log_error("[-] Could not load image <%s>\n", filename);
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



// END OF [FUNCTION IMPLEMENTATIONS]