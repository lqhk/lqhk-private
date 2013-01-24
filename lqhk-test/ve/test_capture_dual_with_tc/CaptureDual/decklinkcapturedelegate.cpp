#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include <boost/signals2/signal.hpp>

#include "decklink/DeckLinkAPI.h"
#include "decklinkcapturedelegate.h"

DeckLinkCaptureDelegate::DeckLinkCaptureDelegate() :
    m_refCount(0),frameCount(0),maxFrames(-1),videoOutputFile(-1),g_timecodeFormat(0)
{
    pthread_mutex_init(&m_mutex, NULL);
}

DeckLinkCaptureDelegate::DeckLinkCaptureDelegate(int camid, const char *filename) :
    m_refCount(0),frameCount(0),maxFrames(-1),g_timecodeFormat(0)
{
    pthread_mutex_init(&m_mutex, NULL);
    m_CamID = camid;
    if(filename!=NULL)
    {
        snprintf(m_FileName,sizeof(m_FileName),"%s",filename);
        videoOutputFile = open(m_FileName, O_WRONLY|O_CREAT|O_TRUNC, 0664);
    }
    else
    {
        fprintf(stderr, "Requires file name.\n");
    }
}

DeckLinkCaptureDelegate::~DeckLinkCaptureDelegate()
{
    pthread_mutex_destroy(&m_mutex);
}

void DeckLinkCaptureDelegate::SetTimeCodeFormat(const char *timecodeformat)
{
    if (!strcmp(timecodeformat, "rp188"))
        g_timecodeFormat = bmdTimecodeRP188Any;
    else if (!strcmp(timecodeformat, "vitc"))
        g_timecodeFormat = bmdTimecodeVITC;
    else if (!strcmp(timecodeformat, "serial"))
        g_timecodeFormat = bmdTimecodeSerial;
    else
    {
        fprintf(stderr, "Invalid argument: Timecode format \"%s\" is invalid\n", optarg);
        g_timecodeFormat = bmdTimecodeSerial;
    }
}

void DeckLinkCaptureDelegate::SetMaxFrames(int frames)
{
    maxFrames = frames;
}

ULONG DeckLinkCaptureDelegate::AddRef(void)
{
    pthread_mutex_lock(&m_mutex);
        m_refCount++;
    pthread_mutex_unlock(&m_mutex);

    return (ULONG)m_refCount;
}

ULONG DeckLinkCaptureDelegate::Release(void)
{
    pthread_mutex_lock(&m_mutex);
        m_refCount--;
    pthread_mutex_unlock(&m_mutex);

    if (m_refCount == 0)
    {
        delete this;
        return 0;
    }

    return (ULONG)m_refCount;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioFrame)
{
    //TODO
    IDeckLinkVideoFrame*	                rightEyeFrame = NULL;
    IDeckLinkVideoFrame3DExtensions*        threeDExtensions = NULL;
    void*					frameBytes;
    void*					audioFrameBytes;

    // Handle Video Frame
    if(videoFrame)
    {
        // If 3D mode is enabled we retreive the 3D extensions interface which gives.
        // us access to the right eye frame by calling GetFrameForRightEye() .
        if ( (videoFrame->QueryInterface(IID_IDeckLinkVideoFrame3DExtensions, (void **) &threeDExtensions) != S_OK) ||
             (threeDExtensions->GetFrameForRightEye(&rightEyeFrame) != S_OK))
        {
            rightEyeFrame = NULL;
        }

        if (threeDExtensions)
            threeDExtensions->Release();

        if (videoFrame->GetFlags() & bmdFrameHasNoInputSource)
        {
            fprintf(stderr, "Frame received (#%lu) - No input signal detected\n", frameCount);
        }
        else
        {
            const char *timecodeString = NULL;
            if (g_timecodeFormat != 0)
            {
                IDeckLinkTimecode *timecode;
                if (videoFrame->GetTimecode(g_timecodeFormat, &timecode) == S_OK)
                {
                    timecode->GetString(&timecodeString);
                }
            }

            char timecodeStaticString[256];
            snprintf(timecodeStaticString, sizeof(timecodeStaticString), "%s", timecodeString);

            fprintf(stderr, "Frame received (#%lu) [%s] - %s - Size: %li bytes\n",
                    frameCount,
                    timecodeString != NULL ? timecodeString : "No timecode",
                    rightEyeFrame != NULL ? "Valid Frame (3D left/right)" : "Valid Frame",
                    videoFrame->GetRowBytes() * videoFrame->GetHeight());

            fprintf(stderr, "Time Code length: %lu\n", strlen(timecodeStaticString));

            if (timecodeString)
                free((void*)timecodeString);

            if (videoOutputFile != -1)
            {
                videoFrame->GetBytes(&frameBytes);
                write(videoOutputFile, frameBytes, videoFrame->GetRowBytes() * videoFrame->GetHeight());

                if (rightEyeFrame)
                {
                    rightEyeFrame->GetBytes(&frameBytes);
                    write(videoOutputFile, frameBytes, videoFrame->GetRowBytes() * videoFrame->GetHeight());
                }
                write(videoOutputFile, timecodeStaticString, sizeof(timecodeStaticString));
            }
            else
            {
                fprintf(stderr, "No video output file. Dry run.\n");
                videoFrame->GetBytes(&frameBytes);
                if(rightEyeFrame)
                {
                    rightEyeFrame->GetBytes(&frameBytes);
                }
            }
        }

        if (rightEyeFrame)
            rightEyeFrame->Release();

        frameCount++;

        if (maxFrames > 0 && frameCount >= maxFrames)
        {
            sig_CaptureEnd(m_CamID);
        }
    }

    // Handle Audio Frame
//    if (audioFrame)
//    {
//        if (audioOutputFile != -1)
//        {
//            audioFrame->GetBytes(&audioFrameBytes);
//            write(audioOutputFile, audioFrameBytes, audioFrame->GetSampleFrameCount() * g_audioChannels * (g_audioSampleDepth / 8));
//        }
//    }
    return S_OK;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode *mode, BMDDetectedVideoInputFormatFlags)
{
    return S_OK;
}

boost::signals2::connection DeckLinkCaptureDelegate::subscribe(const slotType &watcher)
{
    return sig_CaptureEnd.connect(watcher);
}
