#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

struct wavFileInfo
{
    /* RIFF chunk descriptor */
    unsigned int chunkID;
    unsigned int chunkSize;
    unsigned int format;
    
    /* fmt sub-chunk */
    unsigned int subchunk1ID;
    unsigned int subchunk1Size;
    unsigned int audioFormat;
    unsigned int numChannels;
    unsigned int sampleRate;
    unsigned int byteRate;
    unsigned int blockAlign;
    unsigned int bitsPerSample;

    /* data sub-chunk */
    unsigned int subchunk2ID;
    unsigned int subchunk2Size;
    /* audio data... */
};

void usage(void)
{
    printf(
        "usage: audiotool <filename> [input format [output format]]\n"
        "available file formats are:\n"
        "\twav\n"
        "\tmp3\n\n"
        "defaults to wav->mp3 if only a filename is given\n"
    );
}

int getFilePtr(const char* filename, FILE** f)
{
    *f = fopen(filename, "r");
    if (!*f)
    {
        /* couldn't opn file :( */
        printf("Couldn't open \'%s\'\n", filename);
        return 0;
    }
    else
    {
        printf("Successfully opened \'%s\'\n", filename);
    }
    return 1;
}

void releaseFilePtr(FILE** f)
{
    if (*f)
    {
        fclose(*f);
    }
}

void littleEndianBufToInt(const char unsigned input[], unsigned int* output, size_t size)
{
    *output = 0;
    for (int i = 0; i < size; i++)
    {
        *output |= input[i] << 8;
    }
}

/* ====================================================== */
/* ===== TODO: fix little endian memory corruption! ===== */
/* ====================================================== */

unsigned int bigEndianBufToInt(const unsigned char input[], size_t size)
{
    unsigned int output = 0;
    for (int i = 0; i < size; i++)
    {
        output <<= 8;
        output |= input[i];
    }
    return output;
}

int readIntFromBuf(const unsigned char buf[], unsigned int* output, int* offset, size_t n, int bigEndian)
{
    int i;
    unsigned char tmp[n];
    for (i = 0; i < n; i++)
    {
        tmp[i] = buf[*offset + (bigEndian ? i : n - i - 1)];
    }
    *offset += n;


    *output = bigEndianBufToInt(tmp, n);

    return i;
}

int decodeWav(const char* filename)
{
    FILE * wavFile = NULL;
    unsigned char wavHeader[44];
    struct wavFileInfo* wavHeaderInfo = calloc(sizeof(struct wavFileInfo), 1);

    if (!getFilePtr(filename, &wavFile))
    {
        return 0;
    }

    fseek(wavFile, 0, SEEK_SET);
    fread(wavHeader, sizeof(char), sizeof(wavHeader), wavFile);

    int offset = 0;
    /* RIFF chunk descriptor */
    readIntFromBuf(wavHeader, &wavHeaderInfo->chunkID,       &offset, 4, 1);
    readIntFromBuf(wavHeader, &wavHeaderInfo->chunkSize,     &offset, 4, 0);
    readIntFromBuf(wavHeader, &wavHeaderInfo->format,        &offset, 4, 1);

    /* fmt sub-chunk */
    readIntFromBuf(wavHeader, &wavHeaderInfo->subchunk1ID,   &offset, 4, 1);
    readIntFromBuf(wavHeader, &wavHeaderInfo->subchunk1Size, &offset, 4, 0);
    readIntFromBuf(wavHeader, &wavHeaderInfo->audioFormat,   &offset, 2, 0);
    readIntFromBuf(wavHeader, &wavHeaderInfo->numChannels,   &offset, 2, 0);
    readIntFromBuf(wavHeader, &wavHeaderInfo->sampleRate,    &offset, 4, 0);
    readIntFromBuf(wavHeader, &wavHeaderInfo->byteRate,      &offset, 4, 0);
    readIntFromBuf(wavHeader, &wavHeaderInfo->blockAlign,    &offset, 2, 0);
    readIntFromBuf(wavHeader, &wavHeaderInfo->bitsPerSample, &offset, 2, 0);

    /* data sub-chunk */
    readIntFromBuf(wavHeader, &wavHeaderInfo->subchunk2ID,   &offset, 4, 1);
    readIntFromBuf(wavHeader, &wavHeaderInfo->subchunk2Size, &offset, 4, 0);
    
    
    free(wavHeaderInfo);
    releaseFilePtr(&wavFile);
    return 1;
}

int copyString(char** dest, const char* src)
{
    *dest = calloc(strlen(src) + 1, sizeof(char));
    strcpy(*dest, src);
    return (*dest != NULL) ? 1 : 0;
}

int main(int argc, char** argv)
{
    char* filename = NULL;
    char* formatFrom = NULL;
    char* formatTo = NULL;

    if (argc > 1 && argc < 4)
    {
        filename = argv[1];
        printf("filename: %s\n", filename);

        if (argc > 2)
        {
            formatFrom = argv[2];
        }
        else
        {
            copyString(&formatFrom, "wav");
        }
        printf("source format: %s\n", formatFrom);

        if (argc > 3)
        {
            formatTo = argv[3];
        }
        else
        {
            copyString(&formatTo, "mp3");
        }
        printf("target format: %s\n", formatTo);

        /* -------- cleanup ------- */
        if (*formatFrom)
        {
            free(formatFrom);
        }
        if (*formatTo)
        {
            free(formatTo);
        }
    }
    else
    {
        usage();
        exit(1);
    }

    decodeWav(filename);

    return 0;
}
