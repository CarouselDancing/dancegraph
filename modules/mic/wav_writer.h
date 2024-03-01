#pragma once
#include <ostream>
#include <spdlog/spdlog.h>
namespace WAV_Writer
{
#define SUB_CHUNK_1_SIZE 16

    typedef struct wavfile_header_s
    {
        int8_t    ChunkID[4];     /*  4   */
        int32_t ChunkSize;      /*  4   */
        int8_t    Format[4];      /*  4   */

        int8_t    Subchunk1ID[4]; /*  4   */
        int32_t Subchunk1Size;  /*  4   */
        int16_t AudioFormat;    /*  2   */
        int16_t NumChannels;    /*  2   */
        int32_t SampleRate;     /*  4   */
        int32_t ByteRate;       /*  4   */
        int16_t BlockAlign;     /*  2   */
        int16_t BitsPerSample;  /*  2   */

        int8_t    Subchunk2ID[4];
        int32_t Subchunk2Size;
    } wavfile_header_t;

    int WriteHeader(FILE* outFile, uint32_t numFrames, uint32_t sampleRate, 
        uint32_t numChannels, uint32_t bitsPerSample, uint32_t audioFormat)
    {
        wavfile_header_t wav_header;
        int32_t subchunk2_size = (numFrames * numChannels * bitsPerSample) / 8;
        int32_t chunk_size = 4 + (8 + SUB_CHUNK_1_SIZE) + (8 + subchunk2_size);

        size_t write_count;

        wav_header.ChunkID[0] = 'R';
        wav_header.ChunkID[1] = 'I';
        wav_header.ChunkID[2] = 'F';
        wav_header.ChunkID[3] = 'F';

        wav_header.ChunkSize = chunk_size;

        wav_header.Format[0] = 'W';
        wav_header.Format[1] = 'A';
        wav_header.Format[2] = 'V';
        wav_header.Format[3] = 'E';

        wav_header.Subchunk1ID[0] = 'f';
        wav_header.Subchunk1ID[1] = 'm';
        wav_header.Subchunk1ID[2] = 't';
        wav_header.Subchunk1ID[3] = ' ';

        wav_header.Subchunk1Size = SUB_CHUNK_1_SIZE;
        wav_header.AudioFormat = audioFormat;
        wav_header.NumChannels = numChannels;
        wav_header.SampleRate = sampleRate;
        wav_header.ByteRate = (sampleRate * numChannels * bitsPerSample) / 8;
        wav_header.BlockAlign = (numChannels * bitsPerSample) / 8;
        wav_header.BitsPerSample = bitsPerSample;

        wav_header.Subchunk2ID[0] = 'd';
        wav_header.Subchunk2ID[1] = 'a';
        wav_header.Subchunk2ID[2] = 't';
        wav_header.Subchunk2ID[3] = 'a';
        wav_header.Subchunk2Size = subchunk2_size;

        write_count = fwrite(&wav_header, sizeof(wavfile_header_t), 1, outFile);

        return (write_count != 1) ? -1 : 0;
    }

    static int Write(uint32_t numSamples, uint32_t sampleRate,
        uint32_t numChannels, uint32_t bitsPerSample, uint32_t audioFormat, float* samples, std::string fileName)
    {
        FILE* outFile;

        outFile = fopen(fileName.c_str(), "wb+");
        if (outFile == NULL)
        {
            spdlog::info("Failed to open wav file for writing\n");
            return -1;
        }

        if (WriteHeader(outFile, numSamples, sampleRate, numChannels, bitsPerSample, audioFormat) < 0)
        {
            spdlog::info("Failed to write wav header\n");
            fclose(outFile);  
            return -1;
        }

        int numWritten = fwrite(samples, sizeof(float), numSamples * numChannels, outFile);
        if (numWritten < numSamples * numChannels)
        {
            spdlog::info("Failed to write wav data, num written = {}\n", numWritten);
            fclose(outFile);
            return -1;
        }

        fclose(outFile);
        return 0;
    }
}