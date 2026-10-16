/* Plays the smoke test's MP4 through Media Foundation (as games do): decodes the H.264 video to NV12
 * and the AAC audio to PCM. Prints "OK h264 <w>x<h> frames=<n> luma=<y>" and "OK aac rate=<r>
 * channels=<c>", or FAIL lines. Usage: mf-smoke.exe <path to the MP4> */
#define COBJMACROS
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <stdio.h>

static IMFSourceReader *open_reader(const WCHAR *path, DWORD stream, const GUID *major, const GUID *subtype)
{
    IMFSourceReader *reader = NULL;
    IMFMediaType *type = NULL;
    HRESULT hr;

    if (FAILED(hr = MFCreateSourceReaderFromURL(path, NULL, &reader)))
    {
        printf("FAIL MFCreateSourceReaderFromURL %#lx\n", hr);
        return NULL;
    }
    IMFSourceReader_SetStreamSelection(reader, MF_SOURCE_READER_ALL_STREAMS, FALSE);
    IMFSourceReader_SetStreamSelection(reader, stream, TRUE);
    MFCreateMediaType(&type);
    IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, major);
    IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, subtype);
    hr = IMFSourceReader_SetCurrentMediaType(reader, stream, NULL, type);
    IMFMediaType_Release(type);
    if (FAILED(hr))
    {
        printf("FAIL SetCurrentMediaType %#lx\n", hr);
        IMFSourceReader_Release(reader);
        return NULL;
    }
    return reader;
}

static int check_video(const WCHAR *path)
{
    DWORD stream = MF_SOURCE_READER_FIRST_VIDEO_STREAM, flags, frames = 0, luma = 0;
    IMFSourceReader *reader = open_reader(path, stream, &MFMediaType_Video, &MFVideoFormat_NV12);
    IMFMediaType *type;
    UINT64 size = 0;
    LONGLONG time;

    if (!reader) return 0;
    if (SUCCEEDED(IMFSourceReader_GetCurrentMediaType(reader, stream, &type)))
    {
        IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &size);
        IMFMediaType_Release(type);
    }
    for (;;)
    {
        IMFSample *sample = NULL;
        HRESULT hr = IMFSourceReader_ReadSample(reader, stream, 0, NULL, &flags, &time, &sample);
        if (FAILED(hr))
        {
            printf("FAIL ReadSample (video) %#lx\n", hr);
            break;
        }
        if (sample)
        {
            if (!frames++)
            {
                IMFMediaBuffer *buffer;
                BYTE *data;
                DWORD length;
                if (SUCCEEDED(IMFSample_ConvertToContiguousBuffer(sample, &buffer)))
                {
                    if (SUCCEEDED(IMFMediaBuffer_Lock(buffer, &data, NULL, &length)) && length) luma = data[0];
                    IMFMediaBuffer_Unlock(buffer);
                    IMFMediaBuffer_Release(buffer);
                }
            }
            IMFSample_Release(sample);
        }
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) break;
    }
    IMFSourceReader_Release(reader);
    /* A light-gray picture (RGB 200) is luma ~188 in video range; allow for the encoder. */
    if ((UINT32)(size >> 32) != 128 || (UINT32)size != 96 || frames < 10 || luma < 170 || luma > 205)
    {
        printf("FAIL h264 %ux%u frames=%lu luma=%lu\n", (UINT32)(size >> 32), (UINT32)size, frames, luma);
        return 0;
    }
    printf("OK h264 %ux%u frames=%lu luma=%lu\n", (UINT32)(size >> 32), (UINT32)size, frames, luma);
    return 1;
}

static int check_audio(const WCHAR *path)
{
    DWORD stream = MF_SOURCE_READER_FIRST_AUDIO_STREAM, flags = 0;
    IMFSourceReader *reader = open_reader(path, stream, &MFMediaType_Audio, &MFAudioFormat_PCM);
    IMFMediaType *type;
    IMFSample *sample = NULL;
    UINT32 rate = 0, channels = 0;
    LONGLONG time;
    HRESULT hr;

    if (!reader) return 0;
    if (SUCCEEDED(IMFSourceReader_GetCurrentMediaType(reader, stream, &type)))
    {
        IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate);
        IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_NUM_CHANNELS, &channels);
        IMFMediaType_Release(type);
    }
    hr = IMFSourceReader_ReadSample(reader, stream, 0, NULL, &flags, &time, &sample);
    IMFSourceReader_Release(reader);
    if (FAILED(hr) || !sample || !rate || !channels)
    {
        printf("FAIL aac hr=%#lx sample=%p rate=%u channels=%u\n", hr, (void *)sample, rate, channels);
        if (sample) IMFSample_Release(sample);
        return 0;
    }
    IMFSample_Release(sample);
    printf("OK aac rate=%u channels=%u\n", rate, channels);
    return 1;
}

int main(int argc, char **argv)
{
    WCHAR path[MAX_PATH];
    int ok;

    if (argc != 2 || !MultiByteToWideChar(CP_ACP, 0, argv[1], -1, path, MAX_PATH))
    {
        printf("FAIL usage: mf-smoke.exe <file.mp4>\n");
        return 2;
    }
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_FULL)))
    {
        printf("FAIL MFStartup\n");
        return 1;
    }
    ok = check_video(path);
    ok = check_audio(path) && ok;
    MFShutdown();
    CoUninitialize();
    return ok ? 0 : 1;
}
