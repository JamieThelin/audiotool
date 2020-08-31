#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

struct wavFileInfo
{
    /* RIFF chunk descriptor */
    uint32_t chunkID;
    uint32_t chunkSize;
    uint32_t format;
    
    /* fmt sub-chunk */
    uint32_t subchunk1ID;
    uint32_t subchunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;

    /* data sub-chunk */
    uint32_t subchunk2ID;
    uint32_t subchunk2Size;
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

void littleEndianBufToInt(const uint8_t input[], uintmax_t* output, size_t size)
{
    *output = 0;
    for (size_t i = 0; i < size; i++)
    {
        *output |= input[i] << 8;
    }
}

uintmax_t bigEndianBufToInt(const uint8_t input[], size_t size)
{
    uintmax_t output = 0;
    for (size_t i = 0; i < size; i++)
    {
        output <<= 8;
        output |= input[i];
    }
    return output;
}

size_t readIntFromBuf(const uint8_t buf[], uintmax_t* output, uintmax_t* offset, size_t n, int bigEndian)
{
    size_t i;
    uint8_t tmp[n];
    for (i = 0; i < n; i++)
    {
        tmp[i] = buf[*offset + (bigEndian ? i : n - i - 1)];
    }
    *offset += n;


    *output = bigEndianBufToInt(tmp, n);

    return i;
}

size_t readSignedIntFromBuf(const uint8_t buf[], intmax_t* output, uintmax_t* offset, size_t n, int bigEndian)
{
    size_t i;
    uint8_t tmp[n];
    for (i = 0; i < n; i++)
    {
        tmp[i] = buf[*offset + (bigEndian ? i : n - i - 1)];
    }
    *offset += n;


    *output = ((intmax_t) (bigEndianBufToInt(tmp, n)));

    return i;
}

int writeWavSignal( struct wavFileInfo * wavHeaderInfo, FILE * wavFile, uintmax_t * globalOffset )
{
    uintmax_t * offset = malloc(sizeof(int));
    uintmax_t samplesToPrint = ( wavHeaderInfo->subchunk2Size / ( wavHeaderInfo->bitsPerSample / 8 ) ) * wavHeaderInfo->numChannels;

    uint8_t sampleBuf[wavHeaderInfo->subchunk2Size];

    fseek(wavFile, *globalOffset, SEEK_SET);
    fread(sampleBuf, sizeof(int8_t), sizeof(sampleBuf), wavFile);
    *offset = 0;

    printf("samples:\n");

    for (uintmax_t sample = 0; sample < samplesToPrint; sample++)
    {
        for (uint16_t channel = 0; channel < wavHeaderInfo->numChannels; channel++)
        {
            intmax_t sampleVal = 0;
            readSignedIntFromBuf(sampleBuf, &sampleVal, offset, wavHeaderInfo->bitsPerSample/8, 0);
            printf("%d\n", (int16_t)sampleVal);
        }
    }

    return 1;
}

int decodeWav(const char* filename)
{
    FILE * wavFile = NULL;
    uint8_t wavHeaderBuf[44];
    struct wavFileInfo* wavHeaderInfo = calloc(sizeof(struct wavFileInfo), 1);

    if (!getFilePtr(filename, &wavFile))
    {
        return 0;
    }

    fseek(wavFile, 0, SEEK_SET);
    fread(wavHeaderBuf, sizeof(unsigned char), sizeof(wavHeaderBuf), wavFile);

    uintmax_t offset = 0;
    /* RIFF chunk descriptor */
    readIntFromBuf(wavHeaderBuf, ((uintmax_t*)&wavHeaderInfo->chunkID),       &offset, 4, 1);
    readIntFromBuf(wavHeaderBuf, ((uintmax_t*)&wavHeaderInfo->chunkSize),     &offset, 4, 0);
    readIntFromBuf(wavHeaderBuf, ((uintmax_t*)&wavHeaderInfo->format),        &offset, 4, 1);

    /* fmt sub-chunk */
    readIntFromBuf(wavHeaderBuf, ((uintmax_t*)&wavHeaderInfo->subchunk1ID),   &offset, 4, 1);
    readIntFromBuf(wavHeaderBuf, ((uintmax_t*)&wavHeaderInfo->subchunk1Size), &offset, 4, 0);
    readIntFromBuf(wavHeaderBuf, ((uintmax_t*)&wavHeaderInfo->audioFormat),   &offset, 2, 0);
    readIntFromBuf(wavHeaderBuf, ((uintmax_t*)&wavHeaderInfo->numChannels),   &offset, 2, 0);
    readIntFromBuf(wavHeaderBuf, ((uintmax_t*)&wavHeaderInfo->sampleRate),    &offset, 4, 0);
    readIntFromBuf(wavHeaderBuf, ((uintmax_t*)&wavHeaderInfo->byteRate),      &offset, 4, 0);
    readIntFromBuf(wavHeaderBuf, ((uintmax_t*)&wavHeaderInfo->blockAlign),    &offset, 2, 0);
    readIntFromBuf(wavHeaderBuf, ((uintmax_t*)&wavHeaderInfo->bitsPerSample), &offset, 2, 0);

    /* data sub-chunk */
    readIntFromBuf(wavHeaderBuf, ((uintmax_t*)&wavHeaderInfo->subchunk2ID),   &offset, 4, 1);
    readIntFromBuf(wavHeaderBuf, ((uintmax_t*)&wavHeaderInfo->subchunk2Size), &offset, 4, 0);
    
    writeWavSignal(wavHeaderInfo, wavFile, &offset);
    
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

    /* ##################################################################### */
    /* ######## TODO: add appropriate filetype conversion selection ######## */
    /* ##################################################################### */

    decodeWav(filename);

    return 0;
}
