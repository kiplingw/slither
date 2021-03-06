/*
  Name:         CaptureThread.h (definition)
  Author:       Kip Warner (Kip@TheVertigo.com)
  Description:  CaptureThread class...
*/

// Multiple include protection...
#ifndef _CAPTURETHREAD_H_
#define _CAPTURETHREAD_H_

// Includes...

    // wxWidgets...
    #include <wx/wx.h>
   
    // 2020/06/10 - updating OpenCV headers
    // OpenCV...
    #include <opencv2/opencv.hpp>
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/imgproc/imgproc_c.h>

// Forward declarations...
class MainFrame;

// CaptureThread class...
class CaptureThread : public wxThread
{
    // Public methods...
    public:

        // Construct thread...
        CaptureThread(MainFrame *_pMainFrame);
        
        // Thread entry point...
        virtual void *Entry();
        
        // Perform post processing...
        void PerformPostProcessing(cv::Mat *pIntelImage);

        // Show the OSD over the frame...
        void ShowOnScreenDisplay(IplImage *pIntelImage);

    // Private stuff...
    private:

        // Pointer to main frame to render on...
        MainFrame      *pMainFrame;
};

#endif
