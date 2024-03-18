# CFlut
A C client for the [pixelflut protocol](https://github.com/defnull/pixelflut).
Made to be minimal, fast and reliable, **CFlut** allows you to have fun and create one pixel at a time.

## What is pixelflut?
Quote from the original repository:
> What happens if you give a bunch of hackers the ability to change pixel colors on a projector screen? See yourself :)
>
> Pixelflut is a very simple (and inefficient) ASCII based network protocol to draw pixels on a screen. You can write a basic client in a single line of shell code if you want, but you only get to change a single pixel at a time. If you want to get rectangles, lines, text or images on the screen you have to implement that functionality yourself. That is part of the game.

## Outline
How does **CFlut** work? Think of this part as a *mini* write-up.
First of all I had to implement the protocol. Don't worry, it only took one function.
Pixelflut commands are pretty simple and easy to understand. In general:
```
HELP: Returns a short introductional help text.
SIZE: Returns the size of the visible canvas in pixel as SIZE <w> <h>.
PX <x> <y> Return the current color of a pixel as PX <x> <y> <rrggbb>.
PX <x> <y> <rrggbb(aa)>: Draw a single pixel at position (x, y) with the specified hex color code. If the color code contains an alpha channel value, it is blended with the current color of the pixel.
```
Well, we only need one of the 4. And that is `PX <x> <y> <rrggbb(aa)>`. It is the command which allows us to draw pixels at certain positions.
```c
/**
 * Structure to represent a color.
 * Each color is represented by its 4 channels: red, green, blue, and alpha.
 */
typedef struct {
    unsigned char r, g, b, a;
} color;

/**
 * Convert a pixel's color to a hexadecimal string.
 * @param x X-coordinate of the pixel.
 * @param y Y-coordinate of the pixel.
 * @param c Color of the pixel.
 * @return A string containing the hexadecimal representation of the pixel's color.
 */
char* hexifyPixel(int x, int y, color *c)
```
Now that we have our protocol, we can start working on some basic functionality.
This includes sockets (to communicate with the Pixelflut server), loading and resizing images, and the main function of our program.

Im not going to cover the socket client implementation (`client/`). <br> 
It was fun in the making but nothing to make a write-up about (just your average C networking stuff).

As for loading and resizing the images, I used the [stb library](https://github.com/nothings/stb) for this. Specifically the `stb_image.h` and `stb_image_resize2.h`

Here is the definitions for the 2 functions.
```c
/**
 * Load image from file.
 * @param filename Path to file.
 * @return An image struct containing the loaded image.
 */
image loadImage(char* filename);
/**
 * Resize an image to the specified dimensions.
 * @param image Pointer to the image to resize.
 * @param width New width of the image.
 * @param height New height of the image.
 * @param channels Number of color channels in the image.
 */
void resizeImage(
    image *image, 
    int width, 
    int height, 
    int channels
);
```
Technically we are ready to make a function to draw our images on the canvas. We are not doing that *yet*.

But why?
We want our client to be fast and thus we need efficient solutions. 

The most straightforward way would be to loop through the (x, y) coordinates of our image, convert and send our pixels to the server.
But this loop would take ages.

*Enter chunks* <br>
We can split our image to equal chunks and assign them to threads.
This way the execution weight will be split accross N number of threads, which should theoretically make our program N times faster (assuming perfect conditions).

```c
/**
 * Structure to represent a chunk of an image.
 * A chunk is represented by its start and end pointers (to the image) , and its x, y coordinates.
 */
typedef struct {
    color *start, *end;
    int x, y;
} chunk;

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
            .start = (color*)(image.originalImage + start * image.width * DEFAULT_CHANNELS), 
            .end = (color*)(image.originalImage + end * image.width * DEFAULT_CHANNELS),
        };
    }
    return chunks;
}
```
As the docstrings say, each chunk is represented by a `chunk` structure, which contains start and end pointers to the actual image data, and a pair of (x, y) coordinates representing the top-left pixel of the chunk in the image.

I have not implemented multi-threding yet, but it should be too big of a hassle and I will start working on it when I get some free time.
