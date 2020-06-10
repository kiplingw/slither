/*
  Name:         AnalysisThread.h (definition)
  Author:       Kip Warner (Kip@TheVertigo.com)
  Description:  AnalysisThread class...
*/

// Multiple include protection...
#ifndef _ANALYSISTHREAD_H_
#define _ANALYSISTHREAD_H_

// Includes...

    // wxWidgets...
    #include <wx/wx.h>
    #include <wx/stopwatch.h>
    
    // OpenCV...
    // $VR$: 2020/06/10 - updating headers for new OpenCV
    #include <opencv2/opencv.hpp>
    #include <opencv2/imgcodecs.hpp>
    
    // Worm tracker...
    #include "WormTracker.h"

// Forward declarations...
class MainFrame;

// AnalysisThread class...
class AnalysisThread : public wxThread
{
    // Public methods...
    public:

        // Attributes...

            // Status update stop watch...
            wxStopWatch         StatusUpdateStopWatch;

        // Methods...

            // Analysis thread constructor...
            AnalysisThread(MainFrame &_Frame);

            // Analyze single image...
            void AnalyzeImage(wxString sPath);

            // Analyze video...
            void AnalyzeVideo(wxString sPath);

            // Thread entry point...
            virtual void *Entry();

            // Analysis thread exit callback...
            void OnExit();

    // Protected members...
    protected:

        // Multithreading mutex lock...
        wxMutex             Mutex;

        // Pointer to main frame to render on...
        MainFrame          &Frame;

        // Capture handle...
        CvCapture          *pCapture;

};

#endif

