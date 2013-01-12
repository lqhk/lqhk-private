#ifndef CAPTUREWATCHER_H
#define CAPTUREWATCHER_H

#include "decklinkcapturedelegate.h"
#include <boost/thread.hpp>
class CaptureWatcher
{
public:
    CaptureWatcher();

    void watch(DeckLinkCaptureDelegate& capture);
    void captureEnd(int camID);
    void startAllCapture();

private:
    boost::mutex mu;
    boost::mutex muCaptureCount;
    boost::condition_variable_any condAllFinished;
    int m_CaptureCount;

};

#endif // CAPTUREWATCHER_H
