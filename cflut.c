// [SET-UP]
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <getopt.h>
#include "libs/client/client.h"
#include "libs/pixutils/pixutils.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "libs/stb_image/stb_image.h"
#include "libs/stb_image/stb_image_resize2.h"
#include "libs/log/log.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define DEFAULT_THREAD_COUNT 4

SOCKET client;
// [FUNCTION IMPLEMENTATIONS]
/**
 * Main function of the program.
 * It loads an image, resizes it, divides it into chunks, and processes each chunk.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 */
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

int main(int argc, char *argv[]) {
    int opt;
    int thread_count = DEFAULT_THREAD_COUNT;
    char *image_path = NULL;
    char *dim = NULL;

    // Parse command-line options
    while ((opt = getopt(argc, argv, "d:t:")) != -1) {
        switch (opt) {
            case 'd':
                dim = optarg;
                break;
            case 't':
                thread_count = atoi(optarg);
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
    int width, height;
    if (dim != NULL && parse_dimensions(dim, &width, &height) != 0) {
        log_error("Dimension is of invalid format or is not provided.");
        return 1;
    }

    image imageStruct = loadImage(image_path);
    resizeImage(&imageStruct, width, height, DEFAULT_CHANNELS);
    chunk *imageChunks = makeChunks(imageStruct, thread_count);

    client = initClient();
    int i;
    for (i=0; i < thread_count; i++) {
        log_info("%i\n", i);
        log_info("X:%i Y:%i\n", imageChunks[i].x, imageChunks[i].y);
        processChunk(imageStruct, imageChunks[i], i, client);
    }

    stbi_image_free(imageStruct.originalImage);
    closesocket(client);
    WSACleanup();
    return 0;

}