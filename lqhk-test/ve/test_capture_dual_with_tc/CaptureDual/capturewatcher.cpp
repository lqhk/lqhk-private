#include "capturewatcher.h"
#include <stdio.h>

CaptureWatcher::CaptureWatcher() :
    m_CaptureCount(0)
{
}

void CaptureWatcher::watch(DeckLinkCaptureDelegate &capture)
{
    boost::mutex::scoped_lock lock(muCaptureCount);
    capture.subscribe(boost::bind(&CaptureWatcher::captureEnd, this, _1));
    ++m_CaptureCount;
    fprintf(stderr, "Add watch. Totally watch %d capture.\n", m_CaptureCount);
}

void CaptureWatcher::captureEnd(int camID)
{
    boost::mutex::scoped_lock lock(muCaptureCount);
    if(m_CaptureCount>0)
    {
        fprintf(stderr,"Camera %d capture ends.\n",camID);
        --m_CaptureCount;
    }
    if(m_CaptureCount==0)
    {
        fprintf(stderr, "All captures end.\n");
        condAllFinished.notify_one();
    }
}

void CaptureWatcher::startAllCapture()
{
    boost::mutex::scoped_lock lock(mu);
    condAllFinished.wait(mu);
}
