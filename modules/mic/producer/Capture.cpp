#include "Capture.h"
#include <algorithm>

#include <spdlog/spdlog.h>

#define REFTIMES_PER_SEC  10

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

Capture::Capture() :
    pAudioClient(NULL),
    pCaptureClient(NULL),
    capturing(false),
    format(NULL),
    bytesAvailable(0),
    clearData(false)
{
}

Capture::~Capture()
{
}

struct DeviceStruct {
    IMMDevice* device;
    std::string name;
};
IMMDevice* Capture::SelectDevice(int deviceIndex, bool defaultDevice) // Selects input device
{
    HRESULT hr;
    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* device = NULL;
    hr = CoCreateInstance(
        CLSID_MMDeviceEnumerator, NULL,
        CLSCTX_ALL, IID_IMMDeviceEnumerator,
        (void**)&pEnumerator);

    if (defaultDevice)
    {
        pEnumerator->GetDefaultAudioEndpoint(eCapture, eMultimedia, &device);
    }
    else
    {
        IMMDeviceCollection* deviceCollection;
        UINT numDevices = 0;

        IPropertyStore* deviceProperties;
        static PROPERTYKEY key;
        GUID IDevice_FriendlyName = { 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 } };
        key.pid = 14;
        key.fmtid = IDevice_FriendlyName;
        PROPVARIANT varName;
        PropVariantInit(&varName);

        hr = pEnumerator->EnumAudioEndpoints(eCapture, eMultimedia, &deviceCollection);
        hr = deviceCollection->GetCount(&numDevices);

        // Get each capture device
        DeviceStruct* devices = new DeviceStruct[numDevices];
        for (int i = 0; i < numDevices; i++)
        {
            deviceCollection->Item(i, &devices[i].device);
            devices[i].device->OpenPropertyStore(STGM_READ, &deviceProperties);

            deviceProperties->GetValue(key, &varName);
            std::wstring ws(varName.pwszVal);

            devices[i].name = std::string(ws.begin(), ws.end());
        }

        // Sort devices array alphabetically to match order in Unity
        std::sort(&devices[0], &devices[numDevices],
            [](const DeviceStruct& a, const DeviceStruct& b) {return a.name < b.name; });

        // Return device based on selected device ID
        device = devices[deviceIndex].device;
        delete[] devices;
    }
    return device;
}

HRESULT Capture::Init(int deviceIndex, bool defaultDevice) // Initialize WASAPI for audio capture
{
    HRESULT hr;
    UINT32 bufferFrameCount;
    IMMDevice* pDevice = SelectDevice(deviceIndex, defaultDevice); // Get output device based on ID selected in Unity

    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->GetMixFormat(&format);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        0,
        REFTIMES_PER_SEC,
        0,
        format,
        NULL);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->GetService(
        IID_IAudioCaptureClient,
        (void**)&pCaptureClient);
    EXIT_ON_ERROR(hr)

    return hr;

Exit:

    Shutdown();
    return hr;
}

HRESULT Capture::Start(std::vector<BYTE>& audioBytes) // Starts audio capture
{
    HRESULT hr;
    UINT32 packetLength = 0;
    UINT32 numFramesAvailable;
    DWORD flags;
    BYTE* pData;
    hr = pAudioClient->Start(); // start recording
    EXIT_ON_ERROR(hr)

    if (SUCCEEDED(hr))
    {
        capturing = true;
    }
    spdlog::info("Format sample rate: {}\n", format->nSamplesPerSec);
    while (capturing)
    {

        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        EXIT_ON_ERROR(hr)

        while (packetLength != 0)
        {
            hr = pCaptureClient->GetBuffer(&pData,
                &numFramesAvailable,
                &flags, NULL, NULL);
            EXIT_ON_ERROR(hr)

            if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
            {
                pData = NULL; // Tell CopyData to write silence
            }
            else if (pData)
            {
                const UINT32 frameSize = (format->wBitsPerSample / 8 * format->nChannels);
                const UINT32 totalSize = numFramesAvailable * frameSize;
                audioBytes.insert(audioBytes.end(), pData, pData + totalSize);
            }
            
            hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
            EXIT_ON_ERROR(hr)

            hr = pCaptureClient->GetNextPacketSize(&packetLength);
            EXIT_ON_ERROR(hr)
        }
    }

    return hr;

Exit:
    Shutdown();
    return hr;
}

void Capture::Stop()
{
    capturing = false;
}

HRESULT Capture::Shutdown()
{
    HRESULT hr = S_OK;

    if (pAudioClient != NULL)
    {
        hr = pAudioClient->Stop();
    }

    if (pCaptureClient != NULL)
    {
        pCaptureClient->Release();
        pCaptureClient = NULL;
    }

    if (pAudioClient != NULL)
    {
        pAudioClient->Release();
        pAudioClient = NULL;
    }

    if (format != NULL)
    {
        CoTaskMemFree(format);
        format = NULL;
    }

    return hr;
}

WAVEFORMATEX* Capture::GetFormat()
{
    return format;
}