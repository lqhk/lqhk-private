#include <iostream>
#include <boost/program_options.hpp>
#include "boost_opt_util.h"
#include "decklink/DeckLinkAPI.h"
#include "decklinkcapturedelegate.h"
#include "capturewatcher.h"

//  Amount of DeckLink ports. For example, there are two ports on the
//   DeckLink Duo, so the amount is set to 2.
const int g_MaxDeckLink = 2;
int								audioOutputFile = -1;

//IDeckLink 						*deckLink;
//IDeckLinkDisplayModeIterator	*displayModeIterator = NULL;

static BMDTimecodeFormat		g_timecodeFormat = 0;

/*!
 * \brief g_videoModeIndex
 * An int to specify the video mode.
 * Value following mode,m parameter.
 */
static int						g_videoModeIndex = -1;
static int						g_audioChannels = 2;
static int						g_audioSampleDepth = 16;
std::string*         			g_videoOutputFile = NULL;
std::string*					g_audioOutputFile = NULL;
static int						g_maxFrames = -1;

static unsigned long 			frameCount = 0;
using namespace std;

std::string printModeID()
{
    IDeckLinkIterator *m_DeckLinkIterator = CreateDeckLinkIteratorInstance();
    IDeckLink *m_DeckLink;
    IDeckLinkInput *m_DeckLinkInput;
    IDeckLinkDisplayModeIterator *m_DisplayModeIterator = NULL;
    HRESULT result;
    IDeckLinkDisplayMode *m_DisplayMode = NULL;
    int m_DisplayModeCount = 0;

    std::string strModeHelp("Define Mode.\n");

    fprintf(stderr,
            "Usage: Capture -m <mode id> [OPTIONS]\n"
            "\n"
            "    -m <mode id>:\n"
            );
    try
    {
        if(!m_DeckLinkIterator)
        {
            result = E_FAIL;
            fprintf(stderr, "Warning: This application requires the DeckLink drivers installed.\n");
            throw result;
            //return strModeHelp;
        }
        result = m_DeckLinkIterator->Next(&m_DeckLink);
        if(result != S_OK)
        {
            fprintf(stderr, "No DeckLink PCI cards found.\n");
            throw result;
            //return strModeHelp;
        }
        result = m_DeckLink->QueryInterface(IID_IDeckLinkInput, (void**)&m_DeckLinkInput);
        if(result != S_OK)
        {
            fprinrf(stderr, "Cannot create DeckLinkInput.\n");
            throw result;
            //return strModeHelp;
        }
        /*! In standard procedure, the initialization requires a
         *  delegate. The codes are listed below. Please notice that
         *  the DeckLinkCaptureDelegate class is a customed class which
         *  inherits from IDeckLinkInputCallback.
         *
         *  DeckLinkCaptureDelegate *delegate = new DeckLinkCaptureDelegate();
         *  m_DeckLinkInput->SetCallback(delegate);
         *
         */


        result = m_DeckLinkInput->GetDisplayModeIterator(&m_DisplayModeIterator);
        if(result != S_OK)
        {
            fprintf(stderr, "Could not obtain the video output display mode iterator - result = %08x\n", result);
            strModeHelp.append("Warning: Please check DeckLink hardware or driver!\n");
            throw result;
            //return strModeHelp;
        }

        //  Start to enumerate all supported display modes.
        while (m_DisplayModeIterator->Next(&m_DisplayMode) == S_OK)
        {
            char *displayModeString = NULL;

            result = m_DisplayMode->GetName((const char **) &displayModeString);
            if (result == S_OK)
            {
                BMDTimeValue frameRateDuration, frameRateScale;
                m_DisplayMode->GetFrameRate(&frameRateDuration, &frameRateScale);

                char strModeLine[512];
                snprintf(strModeLine, sizeof(strModeLine),
                         "        %2d:  %-20s \t %li x %li \t %g FPS\n",
                         m_DisplayModeCount, displayModeString, displayMode->GetWidth(),
                         displayMode->GetHeight(), (double)frameRateScale / (double)frameRateDuration);
                free(displayModeString);
                +m_DisplayModeCount;
                strModeHelp.append(strModeLine);
            }

            // Release the IDeckLinkDisplayMode object to prevent a leak
            m_DisplayMode->Release();
        }
        m_DisplayModeIterator->Release();
        m_DisplayModeIterator = NULL;
    }
    catch(HRESULT)
    {
        //exception
        fprintf(stderr, "An exception happened while enumerating display modes.\n");
    }
    /*!
     * After catch the exceptions, the application will execute the
     * rest of codes. It looks like a "finally{}" in c# or java.
     */
    if(m_DisplayMode!=NULL)
    {
        m_DisplayMode->Release();
        m_DisplayMode = NULL;
    }
    if(m_DisplayModeIterator!=NULL)
    {
        m_DisplayModeIterator->Release();
        m_DisplayModeIterator = NULL;
    }
    if(m_DeckLinkInput!=NULL)
    {
        m_DeckLinkInput->Release();
        m_DeckLinkInput = NULL;
    }
    if(m_DeckLink!=NULL)
    {
        m_DeckLink->Release();
        m_DeckLink = NULL;
    }
    if(m_DeckLinkIterator!=NULL)
    {
        m_DeckLinkIterator->Release();
        m_DeckLinkIterator = NULL;
    }
    return strModeHelp;
}

int main(int argc, char * argv[])
{
    /*!
     * \brief m_DeckLinkIterator
     *Object for enumerate every DeckLink port. This object
     *is used for checking DeckLink drivers later.
     */
    IDeckLinkIterator			*g_DeckLinkIterator;

    /*!
     * Initialization checking steps.
     */

    g_DeckLinkIterator = CreateDeckLinkIteratorInstance();
    IDeckLink *m_DeckLink = NULL;
    if (!g_DeckLinkIterator)
    {
        fprintf(stderr, "This application requires the DeckLink drivers installed.\n");
    }

    /*!
     *Enumerate and check all DeckLink ports. If the count is 0,
     *the application will not run. However, the help information
     *is able to display.
     */
    int totalDeckLink = 0;
    while (g_DeckLinkIterator->Next(&m_DeckLink) == S_OK)
    {
        ++totalDeckLink;
        //  Release before enumerate next port.
        m_DeckLink->Release();
        m_DeckLink = NULL;
    }
    /*!
     *Release IDeckLinkIterator for the function printModeID.
     *That function will enumerate all supported display modes.
     */
    g_DeckLinkIterator->Release();
    g_DeckLinkIterator = NULL;

    /*!
     *Variables below are changed to vector to support dual video inputs.
     *
     *  BMDVideoInputFlags			inputFlags = 0;
     *  BMDDisplayMode				selectedDisplayMode = bmdModeNTSC;
     *  BMDPixelFormat				pixelFormat = bmdFormat8BitYUV;
     *  int							exitStatus = 1;
     *  bool 						foundDisplayMode = false;
     */


    /*!
     * \brief g_PixelFormat
     * Pixel format.
     * 0=bmdFormat8BitYUV
     * 1=bmdFormat10BitYUV
     * 2=bmdFormat10BitRGB
     */
    BMDPixelFormat g_PixelFormat = bmdFormat8BitYUV;
    HRESULT						result;

    /*!
     *Application options.
     */
    boost::program_options::options_description cmdline("Command line options");
    std::string strMode(printModeID());
    cmdline.add_options()
            ("help,h", "show help message")
            ("mode,m",
             boost::program_options::value<int>()->default_value(-1),
             strMode.c_str())
            ("pixel,p",
             boost::program_options::value<unsigned int>()->default_value(0),
             "<pixelformat>\n  0:  8 bit YUV (4:2:2) (default)\n  1:  10 bit YUV (4:2:2)\n  2:  10 bit RGB (4:4:4)\n")
            ("timecode,t",
             boost::program_options::value<std::string>(),
             "Define timecode format\n  rp188:  RP 188\n  vitc:  VITC\n  serial:  Serial Timecode\n")
            ("filename,f",
             boost::program_options::value<std::string>(),
             "Filename raw video will be written to")
            ("audio,a",
             boost::program_options::value<std::string>(),
             "Filename raw audio will be written to")
            ("channels,c",
             boost::program_options::value<unsigned int>()->default_value(2),
             "Audio Channels (2, 8 or 16 - default is 2)")
            ("depth,s",
             boost::program_options::value<unsigned int>()->default_value(16),
             "Audio Sample Depth (16 or 32 - default is 16)")
            ("num,n",
             boost::program_options::value<int>()->default_value(-1),
             "Number of frames to capture (default is -1 unlimited)")
    ;
    boost::program_options::variables_map parameter_map = boost_opt_check(cmdline, argc, argv);
    fprintf(stderr, "Command line options:\n%s\n", boost_opt_to_string(cmdline, parameter_map, "  ").c_str());

    g_videoModeIndex = parameter_map["mode"].as<int>();

    switch(parameter_map["pixel"].as<unsigned int>())
    {
    case 0: g_PixelFormat = bmdFormat8BitYUV; break;
    case 1: g_PixelFormat = bmdFormat10BitYUV; break;
    case 2: g_PixelFormat = bmdFormat10BitRGB; break;
    }

    std::string strTimeCode = boost_opt_string(parameter_map, "timecode");
    char strConstTimeCode[128];
    snprintf(strConstTimeCode,sizeof(strConstTimeCode),strTimeCode.c_str());
    // Comment lines below are previously for time code.
    // Now the time code is a const char* parameter of DeckLinkCaptureDelegate().
//    if (strTimeCode.compare("rp188"))
//        g_timecodeFormat = bmdTimecodeRP188Any;
//    else if (strTimeCode.compare("vitc"))
//        g_timecodeFormat = bmdTimecodeVITC;
//    else if (strTimeCode.compare("serial"))
//        g_timecodeFormat = bmdTimecodeSerial;

    g_videoOutputFile = new std::string(boost_opt_string(parameter_map, "filename"));
    g_audioOutputFile = new std::string(boost_opt_string(parameter_map, "audio"));

    g_audioChannels = parameter_map["channels"].as<unsigned int>();
    if (g_audioChannels != 2 &&  g_audioChannels != 8 && g_audioChannels != 16)
        g_audioChannels = 2;

    g_audioSampleDepth = parameter_map["depth"].as<unsigned int>();
    if(g_audioSampleDepth != 16 && g_audioSampleDepth != 32)
        g_audioSampleDepth = 16;

    g_maxFrames = parameter_map["num"].as<int>();

    if(totalDeckLink==0)
    {
        fprintf(stderr, "Error: No DeckLink card. Application exit.\n");
        return 0;
    }
    else if(totalDeckLink!=g_MaxDeckLink)
    {
        fprintf(stderr, "Warning: Assume %d DeckLink ports. Actual amount is %d.\n", g_MaxDeckLink, totalDeckLink);
    }

    std::vector<BMDVideoInputFlags> inputFlags;
    std::vector<BMDDisplayMode> selectedDisplayMode;
    std::vector<BMDPixelFormat> pixelFormat;
    std::vector<bool> foundDisplayMode;

    //  Release port iterator first and then enumerate all ports again for
    //   real capture.
    deckLinkIterator = CreateDeckLinkIteratorInstance();
    if(m_DeckLink!=NULL)
    {
        m_DeckLink->Release();
        m_DeckLink = NULL;
    }
    /*!
     * \brief deckLink
     * Interface for a physical DeckLink device.
     */
    std::vector<IDeckLink *> deckLink;

    /*!
     * \brief deckLinkInput
     * Interface for an application to capture a video and audio stream.
     */
    std::vector<IDeckLinkInput *> deckLinkInput;

    /*!
     * \brief delegate
     * DeckLinkCaptureDelegate. A callback class which is called for
     * each captured frame.
     */
    std::vector<DeckLinkCaptureDelegate *> delegate;

    IDeckLinkDisplayModeIterator *m_DisplayModeIterator = NULL;

    /*!
     * \brief displayMode
     * Interface represented a supported display mode.
     * Requires a IDeckLinkDisplayModeIterator to enumerate
     * all supported display modes.
     */
    std::vector<IDeckLinkDisplayMode *> displayMode;

    int i_DeckLink = 0;
    while(deckLinkIterator->Next(&m_DeckLink)==S_OK)
    {
        deckLink.push_back(m_DeckLink);
        //Initialize DeckLink ports.
        IDeckLinkInput *m_DeckLinkInput;
        m_DeckLink->QueryInterface(IID_IDeckLinkInput, (void**)&m_DeckLinkInput);//Requires release if failed.
        deckLinkInput.push_back(m_DeckLinkInput);



        DeckLinkCaptureDelegate *m_Delegate;
        std::string m_VideoOutputFile(*g_videoOutputFile);
        char m_istr[8];
        snprintf(m_istr,sizeof(m_istr),"%d",i_DeckLink);
        m_VideoOutputFile.append(m_istr);
        m_VideoOutputFile.append(".hdraw");


        //  Variables for confirming display mode.
        BMDVideoInputFlags m_InputFlags = 0; //Ignore. BMDVideoInputFlags are mainly for 3D supports.
        BMDDisplayMode m_SelectedDisplayMode = bmdModeNTSC;
        BMDPixelFormat m_PixelFormat = bmdFormat8BitYUV;
        bool m_FoundDisplayMode = false;
        //  Enumerate all display modes supported by this DeckLinkInput
        result = m_DeckLinkInput->GetDisplayModeIterator(&m_DisplayModeIterator);
        if(result != S_OK)
        {
            fprintf(stderr, "Could not obtain the video output display mode iterator - result = %08x\n", result);
            //exit
        }
        IDeckLinkDisplayMode *m_DisplayMode = NULL;
        int displayModeCount = 0;
        while(m_DisplayModeIterator->Next(&m_DisplayMode)==S_OK)
        {
            if(g_videoModeIndex==displayModeCount)
            {
                BMDDisplayModeSupport result;
                const char *displayModeName;

                m_FoundDisplayMode = true;
                m_DisplayMode->GetName(&displayModeName);
                m_SelectedDisplayMode = m_DisplayMode->GetDisplayMode();

                m_DeckLinkInput->DoesSupportVideoMode(m_SelectedDisplayMode,
                                                      m_PixelFormat,
                                                      bmdVideoInputFlagDefault,
                                                      &result,
                                                      NULL);
                if(result==bmdDisplayModeNotSupported)
                {
                    fprintf(stderr, "The display mode %s is not supported with the selected pixel format on input %d.\n",
                            displayModeName, i_DeckLink);
                    //exit
                }
                if(m_InputFlags & bmdVideoInputDualStream3D)
                {
                    if(!(m_DisplayMode->GetFlags() & bmdDisplayModeSupports3D))
                    {
                        fprintf(stderr, "The display mode %s is not supported with 3D on input %d.\n",
                                displayModeName, i_DeckLink);
                        //exit
                    }
                }

                displayMode.push_back(m_DisplayMode);   //Acutally, displayMode is not necessary
                                                        // for future.
                inputFlags.push_back(m_InputFlags);
                selectedDisplayMode.push_back(m_SelectedDisplayMode);
                pixelFormat.push_back(m_PixelFormat);
                foundDisplayMode.push_back(m_FoundDisplayMode);
                break;
            }
            ++displayModeCount;
            m_DisplayMode->Release();
        }
        if(!m_FoundDisplayMode)
        {
            fprintf(stderr, "Invalid mode %d specified on input %d.\n", g_videoModeIndex, i_DeckLink);
            //exit
        }     
        m_Delegate = new DeckLinkCaptureDelegate(i_DeckLink, m_VideoOutputFile.c_str());
        m_Delegate->SetTimeCodeFormat(strConstTimeCode);
        if(g_maxFrames>0)
        {
            m_Delegate->SetMaxFrames(g_maxFrames);
        }
        delegate.push_back(m_Delegate);
        m_DeckLinkInput->SetCallback(m_Delegate);
        m_DeckLink = NULL;
    }
    //  Check amount of variables.
    if(deckLink.size()!=totalDeckLink)
    {
        fprintf(stderr, "Some of the DeckLink card failed. Exit.\n");
        return 0;
        //exit
    }
    if(deckLinkInput.size()!=totalDeckLink)
    {
        fprintf(stderr, "Some of the DeckLink input failed. Exit.\n");
        return 0;
        //exit
    }
    if(delegate.size()!=totalDeckLink)
    {
        fprintf(stderr, "Some of the DeckLink capture delegate failed. Exit.\n");
        return 0;
        //exit
    }

    CaptureWatcher watcher;
    for(int i_AddWatch=0;i_AddWatch<totalDeckLink;++i_AddWatch)
    {
        DeckLinkCaptureDelegate *m_Delegate = NULL;
        m_Delegate = delegate.at(i_AddWatch);
        watcher.watch(*m_Delegate);
    }
    for(int i_EnableInput=0;i_EnableInput<totalDeckLink;++i_EnableInput)
    {
        IDeckLinkInput *m_DeckLinkInput = NULL;
        BMDDisplayMode *m_DisplayMode = &(selectedDisplayMode.at(i_EnableInput));
        BMDPixelFormat *m_PixelFormat = &(pixelFormat.at(i_EnableInput));
        BMDVideoInputFlags *m_InputFlags = &(inputFlags.at(i_EnableInput));
        m_DeckLinkInput = deckLinkInput.at(i_EnableInput);
        result = m_DeckLinkInput->EnableVideoInput(*m_DisplayMode, *m_PixelFormat, *m_InputFlags);
        if(result!=S_OK)
        {
            fprintf(stderr, "Failed to enable video input on input %d\n", i_EnableInput);
            //exit
        }
        resutl = m_DeckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz, g_audioSampleDepth, g_audioChannels);
        if(result!=S_OK)
        {
            fprintf(stderr, "Failed to enable audio input on input %d\n", i_EnableInput);
            //exit
        }
    }
    for(int i_StartStream=0;i_StartStream<totalDeckLink;++i_StartStream)
    {
        IDeckLinkInput *m_DeckLinkInput = NULL;
        m_DeckLinkInput = deckLinkInput.at(i_StartStream);
        result = m_DeckLinkInput->StartStreams();
        if(result != S_OK)
        {
            fprintf(stderr, "Cannot start streams on input %d.\n",i_StartStream);
            //exit
        }
    }
    watcher.startAllCapture();

    for(int i_Release=0;i_Release<displayMode.size();++i_Release)
    {
        //Release
        IDeckLinkDisplayMode *m_DisplayMode = displayMode.at(i_Release);
        if(m_DisplayMode!=NULL)
        {
            m_DisplayMode->Release();
            m_DisplayMode = NULL;
        }
    }
    displayMode.clear();
    for(int i_Release=0;i_Release<deckLinkInput.size();++i_Release)
    {
        //Release
        IDeckLinkInput *m_DeckLinkInput = deckLinkInput.at(i_Release);
        if(m_DeckLinkInput!=NULL)
        {
            m_DeckLinkInput->Release();
            m_DeckLinkInput = NULL;
        }
    }
    deckLinkInput.clear();
    for(int i_Release=0;i_Release<deckLink.size();++i_Release)
    {
        //Release
        IDeckLinkInput *m_DeckLink = deckLink.at(i_Release);
        if(m_DeckLink!=NULL)
        {
            m_DeckLink->Release();
            m_DeckLink = NULL;
        }
    }
    deckLink.clear();

    return 0;
}



