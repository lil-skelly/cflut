// [SET-UP]
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <getopt.h>
#include <pthread.h>

#include "libs/client/client.h"
#include "libs/pixutils/pixutils.h"

#include "libs/threadpool/threadpool.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "libs/stb_image/stb_image.h"
#include "libs/stb_image/stb_image_resize2.h"
#include "libs/log/log.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define DEFAULT_WIDTH 400
#define DEFAULT_HEIGHT 400

#define DEFAULT_THREAD_COUNT 4
#define DEFAULT_QUEUE_SIZE 256

SOCKET client;
// [FUNCTION IMPLEMENTATIONS]
/**
 * Main function of the program.
 * It loads an image, resizes it, divides it into chunks, and processes each chunk.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 */
int parse_dimensions(char *dim, int *width, int *height);
threadpool_t* hThreadpool(int thread_count, int queue_size, int flags);


int parse_dimensions(char *dim, int *width, int *height) {
    char *token = strtok(dim, ":");
    if (token != NULL) {
        *width = atoi(token);
        token = strtok(NULL, ":");
        if (token != NULL) {
            *height = atoi(token);
        } else {
            log_error("Invalid dimension format: %s\n", dim);
            return 1;
        }
    }
    return 0;
}

threadpool_t* hThreadpool(int thread_count, int queue_size, int flags){
    threadpool_t *pool = threadpool_create(thread_count, queue_size, flags);
    if (pool == NULL) {
        log_fatal("Unable to initialize thread pool.");
        return 1;
    }
    log_info("Pool started with %d threads and queue size of %d\n", thread_count, queue_size);
    return pool;
}

int main(int argc, char *argv[]) {
    client = initClient();
    int thread_count = DEFAULT_THREAD_COUNT;
    int queue_size = DEFAULT_QUEUE_SIZE;

    threadpool_t *pool;
    
    int opt;
    char *image_path = NULL;
    char *dim = NULL;
    int width;
    int height;
    
    // Parse command-line options
    while ((opt = getopt(argc, argv, "d:t:q:")) != -1) {
        switch (opt) {
            case 'd':
                dim = optarg;
                break;
            case 't':
                thread_count = atoi(optarg);
                break;
            case 'q':
                queue_size = atoi(optarg);
                break;
            default:
                log_error("Usage: %s [-d width:height] [-t threads] <image_path>\n", argv[0]);
                return 1;
        }
    }

    if (optind < argc) {
        image_path = argv[optind];
    } else {
        log_error("Usage: %s [-d width:height] [-t threads] <image_path>\n", argv[0]);
        return 1;
    }

    // Get dimension
    if (dim != NULL && parse_dimensions(dim, &width, &height) != 0) {
        log_error("[-] Dimension is of invalid format or is not provided.");
        return 1;
    }

    image imageStruct = loadImage(image_path);
    resizeImage(&imageStruct, width, height, DEFAULT_CHANNELS);
    chunk *imageChunks = makeChunks(imageStruct, thread_count);

    int i;
    pool = hThreadpool(thread_count, queue_size, 0);

    processArgs* argsArray = malloc(sizeof(processArgs) * thread_count);
    if(argsArray == NULL) {
        log_fatal("[-x-] Unable to allocate memory for argsArray\n");
        return 1;
    }

    for (i=0; i < thread_count; i++) {
        argsArray[i].image = imageStruct;
        argsArray[i].chunk = imageChunks[i];
        argsArray[i].client = client;

        if (threadpool_add(pool, processChunk, &argsArray[i], 0) != 0) {
            log_fatal("[-x-] Error adding task for chunk %p", (void*)argsArray[i].chunk.start);
            return 1;
        }
        log_info("[*] Processing (%i): %p", i+1, (void*)imageChunks[i].start);
    }
    threadpool_destroy(pool, 0);
    free(argsArray);
    free(imageChunks);

    stbi_image_free(imageStruct.originalImage);
    
    closesocket(client);
    WSACleanup();
    
    return 0;
}