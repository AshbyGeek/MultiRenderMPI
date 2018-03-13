#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <cmath>
#include <chrono>

#define UINT_MAX 0xFFFFFFFF

#include "image.hh"
#include "libPngWrapper.hh"

using namespace std;
using namespace std::chrono;

const int MAX_STRING = 200;

const int WIDTH = 1920 * 2;
const int HEIGHT = 1080 * 2;
const int PADDING = 2;

const int numRuns = 4;

const int PIXEL_BUFFER_TYPE_TAG = 1; //Tag to use to indicate that a message contains a pixel buffer type
union PixelBufferType {
    unsigned int buffer[3];
    struct
    {
        unsigned int alpha;
        unsigned int x;
        unsigned int y;
    };
} PixelBuffer;

union LineBufferType {
    unsigned int buffer[4];
    Line line;
} LineBuffer;

void mpiRenderMaster(int rank, int worldSize, std::vector<Line> *lines, Image *image, Pixel color)
{
    printf("Starting to render lines.\n");
    for (unsigned int i = 0; i < lines->size(); i++)
    {
        // TODO: announce the line we're going to work on.
        printf("Broadcasting Line: (%d,%d) to (%d,%d)\n", LineBuffer.line.start.x, LineBuffer.line.start.y, LineBuffer.line.end.x, LineBuffer.line.end.y);
        LineBuffer.line = lines->at(i);
        MPI_Bcast(&LineBuffer.buffer, sizeof(LineBuffer.buffer), MPI_UNSIGNED, 0, MPI_COMM_WORLD);
        printf("Successful Broadcast\n");

        // Receive pixel information from slaves
        int numThreadsDone = 0;
        while (numThreadsDone < worldSize - 1)
        {
            MPI_Status status;
            MPI_Recv(&PixelBuffer.buffer, sizeof(PixelBuffer.buffer), MPI_UNSIGNED, MPI_ANY_SOURCE, PIXEL_BUFFER_TYPE_TAG, MPI_COMM_WORLD, &status);

            if (PixelBuffer.x > image->width || PixelBuffer.y > image->height)
            {
                printf("Thread %d has finished\n", status.MPI_SOURCE);
                numThreadsDone++;
            }
            else
            {
                auto pixel = image->GetPixel(PixelBuffer.x, PixelBuffer.y);
                color.a = PixelBuffer.alpha;
                *pixel = Image::BlendPixels(*pixel, color);
            }
        }
        printf("All threads have completed.\n");
    }
}

void DrawPixelsAt(Line line, unsigned int x, unsigned int y)
{
    PixelBuffer.alpha = Image::RenderAlphaForPixel(line, x, y);
    PixelBuffer.x = x;
    PixelBuffer.y = y;
    MPI_Send(&PixelBuffer.buffer, sizeof(PixelBuffer.buffer), MPI_UNSIGNED, 0, PIXEL_BUFFER_TYPE_TAG, MPI_COMM_WORLD);
}

void DrawLineXCentric(Line line, int slaveRank, int numSlaves)
{
    Point start = line.start;
    Point end = line.end;
    if (line.start.y > line.end.y)
    {
        start = line.end;
        end = line.start;
    }

    int dx = end.x - start.x;
    int dy = end.y - start.y;

    unsigned int chunkSize = std::max(dy / numSlaves, 1);
    unsigned int startY = start.y + chunkSize * slaveRank;
    unsigned int endY = start.y + chunkSize * (slaveRank + 1);

    for (unsigned int y = startY; y <= end.y && y < endY; y++)
    {
        float x = start.x + dx / (float)dy * (y - start.y);
        DrawPixelsAt(line, ceil(x), y);
        DrawPixelsAt(line, floor(x), y);
    }
}

void DrawLineYCentric(Line line, int slaveRank, int numSlaves)
{
    Point start = line.start;
    Point end = line.end;
    if (line.start.x > line.end.x)
    {
        start = line.end;
        end = line.start;
    }

    int dx = end.x - start.x;
    int dy = end.y - start.y;

    unsigned int chunkSize = std::max(dx / numSlaves, 1);
    unsigned int startX = start.x + chunkSize * slaveRank;
    unsigned int endX = start.x + chunkSize * (slaveRank + 1);

    for (unsigned int x = startX; x <= end.x && x < endX; x++)
    {
        float y = start.y + dy / (float)dx * (x - start.x);
        DrawPixelsAt(line, x, ceil(y));
        DrawPixelsAt(line, x, floor(y));
    }
}

void mpiRenderSlave(int rank, int worldSize)
{
    bool run = true;
    while (run)
    {
        // Participate in the broadcast, tells us the next line to compute
        MPI_Bcast(&LineBuffer.buffer, sizeof(LineBuffer.buffer), MPI_UNSIGNED, 0, MPI_COMM_WORLD);
        if (LineBuffer.line.start.x == UINT_MAX && LineBuffer.line.start.y == UINT_MAX)
        {
            run = false;
        }
        else
        {
            auto line = LineBuffer.line;
            int tmpdx = line.end.x - line.start.x;
            int tmpdy = line.end.y - line.start.y;

            if (abs(tmpdx) < abs(tmpdy))
            {
                DrawLineXCentric(line, rank - 1, worldSize - 1);
            }
            else
            {
                DrawLineYCentric(line, rank - 1, worldSize - 1);
            }

            // Indicate that we're done
            PixelBuffer.x = UINT_MAX;
            PixelBuffer.y = UINT_MAX;
            MPI_Send(&PixelBuffer.buffer, sizeof(&PixelBuffer.buffer), MPI_UNSIGNED, 0, PIXEL_BUFFER_TYPE_TAG, MPI_COMM_WORLD);
        }
    }
}

std::vector<Line> *BuildLines()
{
    std::vector<Line> *lines = new std::vector<Line>();
    Line line;

    // diagonal top left to bottom right
    line.start.x = PADDING;
    line.start.y = PADDING;
    line.end.x = WIDTH - PADDING;
    line.end.y = HEIGHT - PADDING;
    lines->push_back(line);

    // diagonal top right to bottom left
    line.start.x = WIDTH;
    line.start.y = 0;
    line.end.x = 0;
    line.end.y = HEIGHT;
    lines->push_back(line);

    // diagonal mid top left to mid bottom right (1/3 slope)
    line.start.x = 0;
    line.start.y = (HEIGHT / 3);
    line.end.x = WIDTH;
    line.end.y = (HEIGHT * 2 / 3);
    lines->push_back(line);

    // vertical line
    line.start.x = WIDTH / 2;
    line.end.x = WIDTH / 2;
    line.start.y = PADDING;
    line.end.y = HEIGHT - PADDING;
    lines->push_back(line);

    return lines;
}

int main()
{
    // Setup
    int worldSize;
    int rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
    {
        // Main Process setup
        std::vector<Line> *lines = BuildLines();

        std::vector<Image *> images(numRuns);
        for (int i = 0; i < numRuns; i++)
        {
            images[i] = new Image(WIDTH, HEIGHT);
            images[i]->FillColor(Pixel(100, 100, 100));
        }

        Pixel color;
        color.r = 0;
        color.g = 0;
        color.b = 0;

        // Do the work and time it
        auto t1 = high_resolution_clock::now();

        for (int i = 0; i < numRuns; i++)
        {
            mpiRenderMaster(rank, worldSize, lines, images[i], color);
        }

        auto t2 = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(t2 - t1).count();

        // Stop the work
        LineBuffer.line.start.x = UINT_MAX;
        LineBuffer.line.start.y = UINT_MAX;
        MPI_Bcast(&LineBuffer.buffer, sizeof(LineBuffer.buffer), MPI_UNSIGNED, 0, MPI_COMM_WORLD);

        cout << "Avg runtime (" << numRuns << " runs): ";
        printf("%6.2lf", (duration / 1000 / (float)numRuns));
        cout << " milliseconds\n";

        //write the first image
        LibPngWrapper wrapper("renderedImage.png");
        wrapper.WriteImage(images[0]);

        // Cleanup
        for (unsigned int i = 0; i < images.size(); i++)
        {
            free(images[i]);
        }
        free(lines);
    }
    else
    {
        mpiRenderSlave(rank, worldSize);
    }

    MPI_Finalize();
    return 0;
}