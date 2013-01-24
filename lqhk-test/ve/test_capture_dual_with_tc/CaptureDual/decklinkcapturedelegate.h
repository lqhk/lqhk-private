#ifndef DECKLINKCAPTUREDELEGATE_H
#define DECKLINKCAPTUREDELEGATE_H

#include "decklink/DeckLinkAPI.h"
#include <boost/signals2.hpp>
#include <boost/bind.hpp>

class DeckLinkCaptureDelegate : public IDeckLinkInputCallback
{
    typedef boost::signals2::signal<void(int)> signalType;
    typedef signalType::slot_type slotType;
public:
    DeckLinkCaptureDelegate();
    DeckLinkCaptureDelegate(int camid, const char *filename);
    ~DeckLinkCaptureDelegate(); 

    void SetTimeCodeFormat(const char *timecodeformat);
    void SetMaxFrames(int frames);

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE  Release(void);
    virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags);
    virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*);

    signalType sig_CaptureEnd;
    boost::signals2::connection subscribe(const slotType& watcher);

private:
    ULONG				m_refCount;
    pthread_mutex_t		m_mutex;
    BMDTimecodeFormat		g_timecodeFormat;

    char m_FileName[1024];
    int videoOutputFile;
    int m_CamID;

    unsigned long frameCount;
    int maxFrames;

};

#endif // DECKLINKCAPTUREDELEGATE_H
