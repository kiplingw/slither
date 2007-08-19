/*
  Name:         AnalysisThread.cpp (implementation)
  Author:       Kip Warner (Kip@TheVertigo.com)
  Description:  AnalysisThread class...
*/

// Includes...
#include "AnalysisThread.h"
#include "MainFrame.h"
#include "Experiment.h"
#include "WormTracker.h"

// Constructor locks UI...
AnalysisThread::CAnalysisAutoLock::CAnalysisAutoLock(MainFrame &_Frame)
    : Frame(_Frame)
{
    // Variables...
    wxString    sTemp;

    // Alert OS GUI changes to occur outside main event thread...
    wxMutexGuiEnter();

        // Microscope set...
        Frame.ChosenMicroscopeName->Disable();
        Frame.ChosenMicroscopeTotalZoom->Disable();
        Frame.FieldOfViewDiameter->Disable();

        // Analysis type...
        Frame.ChosenAnalysisType->Disable();

        // Analysis buttons...
        Frame.BeginAnalysisButton->Disable();
        Frame.CancelAnalysisButton->Enable();
    
        // Analysis gauge...
        Frame.AnalysisGauge->SetRange(100);
        Frame.AnalysisGauge->SetValue(0);

        // Clear the status list...
        Frame.AnalysisStatusList->Clear();

        // Alert user...
        sTemp = Frame.ChosenAnalysisType->GetString(
                    Frame.ChosenAnalysisType->GetCurrentSelection()) + 
                wxT(" analysis is running...");
        Frame.AnalysisStatusList->Append(sTemp);

        // Refresh the main frame...
        Frame.Refresh();

    // Done...
    wxMutexGuiLeave(); 
}

// Deconstructor unlocks UI...
AnalysisThread::CAnalysisAutoLock::~CAnalysisAutoLock()
{
    // Alert OS GUI changes to occur outside main event thread...
    wxMutexGuiEnter();

        // Microscope set...
        Frame.ChosenMicroscopeName->Enable();
        Frame.ChosenMicroscopeTotalZoom->Enable();
        Frame.FieldOfViewDiameter->Enable();

        // Analysis type...
        Frame.ChosenAnalysisType->Enable();

        // Analysis buttons...
        Frame.BeginAnalysisButton->Enable();
        Frame.CancelAnalysisButton->Disable();
    
        // Analysis gauge...
        Frame.AnalysisGauge->SetRange(100);
        Frame.AnalysisGauge->SetValue(0);

        // Alert user...
        Frame.AnalysisStatusList->Append(wxT("Analysis ended..."));

        // Refresh the main frame...
        Frame.Refresh();

    // Done...
    wxMutexGuiLeave(); 

    // Stop the analysis timer...
    Frame.AnalysisTimer.Stop();
}

// Analysis thread constructor...
AnalysisThread::AnalysisThread(MainFrame &_Frame)
    : Frame(_Frame),
      pCapture(NULL)
{

}

/* Analyze captured frame and return the analyzed version...
IplImage *AnalysisThread::AnalyzeFrame(IplImage *pImage)
{
    // Variables...
//    CvFont          Font;
//    double          fHorizontalScale    = 1.0;
//    double          fVerticalScale      = 1.0;
//    int             nLineWidth          = 2;
//    wxString        sBuffer;
    IplImage       *pGrayImage          = NULL;
    IplImage       *pThresholdImage     = NULL;
    CvMemStorage   *pContourStorage     = NULL;
    CvContour      *pFirstContour       = NULL;
    IplImage       *pFinalImage         = NULL;

    // Make it grayscale...
    pGrayImage = cvCreateImage(cvGetSize(pImage), 8, 1);
    cvConvertImage(pImage, pGrayImage, CV_BGR2GRAY);

    // Clone the original image...
    pThresholdImage = cvCloneImage(pGrayImage);
    
    // Create a blank final image...
    pFinalImage = cvCreateImage(cvGetSize(pGrayImage), 8, 3);

    // Convert image from gray colour space to colour for final image...
    cvCvtColor(pGrayImage, pFinalImage, CV_GRAY2BGR);

    // To find the contours, we must first threshold the image...
    cvThreshold(pGrayImage, pThresholdImage, 150, 255, CV_THRESH_BINARY);

    // Find the image contours...

        // Allocate storage space for the contours...
        pContourStorage = cvCreateMemStorage(0);

        // Find the contours...
        cvFindContours(pThresholdImage, pContourStorage, 
                       (CvSeq **) &pFirstContour, 
                       sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_NONE, 
                       cvPoint(0, 0));

    // Clear the threshold image...
    cvZero(pThresholdImage);

    // Examine each contour and make note of ones of possible worm size...
    for(CvContour *pCurrentContour = pFirstContour; 
        pCurrentContour;
        pCurrentContour = (CvContour *) pCurrentContour->h_next)
    {
        // Too few vertices...
        if(pCurrentContour->total < 6)
            continue;

        // Calculate the area of the worm...
        double dArea = fabs(cvContourArea(pCurrentContour));

            // Too small / too big to be a worm, discard...
            if((dArea < 400.0) || (1000.0 < dArea))
                continue;

//printf("Worm area: %.0f\r", dArea);

        // Draw it, if enabled...
        if(Frame.ShowVisualsCheckBox->IsChecked())
            cvDrawContours(pFinalImage, (CvSeq *) pCurrentContour, 
                           CV_RGB(239, 34, 0),
                           CV_RGB(239, 34, 0),
                           0,                 
                           2,                 
                           8,                 
                           cvPoint(0, 0));    
    }

    // Show time elapsed...
    
        // Create font...
        cvInitFont(&Font, CV_FONT_HERSHEY_TRIPLEX, fHorizontalScale, 
                   fVerticalScale, 0, nLineWidth);
    
        // Format output...
        int nSeconds = (int) 
            (cvGetCaptureProperty(pCapture, CV_CAP_PROP_POS_MSEC) / 1000.0);
        sBuffer.Printf(wxT("%.2dm %.2ds"), nSeconds / 60, nSeconds % 60);

        // Output...
        cvPutText(pFinalImage, sBuffer.mb_str(), cvPoint(10, 30), &Font, 
                  cvScalar(0, 243, 0));

    // Cleanup...
    cvReleaseMemStorage(&pContourStorage);
    cvReleaseImage(&pGrayImage);
    cvReleaseImage(&pThresholdImage);

    // Return the finally analyzed frame...
    return pFinalImage;
}*/

// Thread entry point...
void *AnalysisThread::Entry()
{
    // Variables...
    WormTracker         Tracker;
    IplImage           *pGrayImage          = NULL;
    IplImage           *pThinkingImage      = NULL;
    long                lLastPulseTime      = ::wxGetLocalTime();
    long                lLastDisplayTime    = 0L;
    wxString            sTemp;
    wxStopWatch         StopWatch;

    // Reconfigure UI for analysis in use...
    CAnalysisAutoLock    AnalysisLock(Frame);

    // Get the complete path to the video to analyze...

        // Find the row selected...
        wxArrayInt SelectedRows = Frame.VideosGrid->GetSelectedRows();
        int nRow = SelectedRows[0];
        
        // Generate complete path...
        wxString sPath = Frame.pExperiment->GetCachePath() +
                         wxT("/videos/") +
                         Frame.VideosGrid->GetCellValue(nRow, 
                                                              MainFrame::TITLE);

    // Initialize capture from AVI...
    pCapture = cvCreateFileCapture(sPath.fn_str());

        // Failed to load video...
        if(!pCapture)
        {
            // Alert...
            wxLogError(wxT("Your system does not appear to have an suitable"
                           " codec installed to read this video."));
            
            // Abort...
            return NULL;
        }

    // Keep showing video until there is nothing left or cancel requested...
    while(cvGrabFrame(pCapture) && !Frame.bExiting && 
          Frame.CancelAnalysisButton->IsEnabled())
    {
/*
printf("%.2f\tCV_CAP_PROP_POS_MSEC\n"
       "%.2f\tCV_CAP_PROP_POS_FRAMES\n"
       "%.2f\tCV_CAP_PROP_POS_AVI_RATIO\n"
       "%.2f\tCV_CAP_PROP_POS_WIDTH\n"
       "%.2f\tCV_CAP_PROP_POS_HEIGHT\n"
       "%.2f\tCV_CAP_PROP_POS_FPS\n"
       "%.2f\tCV_CAP_PROP_FRAME_COUNT\n"
       "%.2f\tCV_CAP_PROP_POS_FOURCC\n\n",
       cvGetCaptureProperty(pCapture, CV_CAP_PROP_POS_MSEC),
       cvGetCaptureProperty(pCapture, CV_CAP_PROP_POS_FRAMES),
       cvGetCaptureProperty(pCapture, CV_CAP_PROP_POS_AVI_RATIO),
       cvGetCaptureProperty(pCapture, CV_CAP_PROP_FRAME_WIDTH),
       cvGetCaptureProperty(pCapture, CV_CAP_PROP_FRAME_HEIGHT),
       cvGetCaptureProperty(pCapture, CV_CAP_PROP_FPS),
       cvGetCaptureProperty(pCapture, CV_CAP_PROP_FRAME_COUNT),
       cvGetCaptureProperty(pCapture, CV_CAP_PROP_FOURCC));*/

        // Retrieve the captured image...
        IplImage const &OriginalImage = *cvRetrieveFrame(pCapture);

        // The tracker prefers grayscale 8-bit unsigned format, prepare...

            // Allocate blank grayscale image...
            pGrayImage = cvCreateImage(cvGetSize(&OriginalImage), 
                                       IPL_DEPTH_8U, 3);

            // Convert original to grayscale...
            cvConvertImage(OriginalImage, pGrayImage);

        // Feed into tracker...
        Tracker.AdvanceNextFrame(*pGrayImage);
        
        // Cleanup gray image...
        cvReleaseImage(&pGrayImage);
        
        // Prepare the thinking image...
        pThinkingImage = cvCloneImage(&Tracker.GetThinkingImage());

            // Cleanup analyzed image as well if buffer is piling up. We can do
            //  this because all we want is the data extrapolated in the 
            //  analyses, with just displaying the visual process to the user as
            //  being merely secondary in priority...
            if(Frame.AnalysisFrameBuffer.size() >= 2 ||
               ::wxGetLocalTime() == lLastDisplayTime)
            {
                // Release the image...
                cvReleaseImage(&pThinkingImage);
            }
                
            // Otherwise, add it to the buffer...
            else
            {
                // Place at the end of the buffer...
                Frame.AnalysisFrameBuffer.push_back(pThinkingImage);
                
                // Remember having stored image for GUI at this interval to
                //  avoid doing it again...
                lLastDisplayTime = ::wxGetLocalTime();
            }
        
        // Depending on processor throttle setting, idle...
        if(Frame.ProcessorThrottle->GetValue() < 100)
        {
            // Compute sleep time from slider value... [1,1000ms]
            int nSleepTime = 
                ((Frame.ProcessorThrottle->GetValue() + 1) / 10.0f);

            // Give up rest of time slice to system for other threads...
            Yield();
            
            // Sleep...
            wxThread::Sleep(nSleepTime);
        }
        
        // Update status...
        
            // Get current position...
            int nCurrentFrame = (int) 
                cvGetCaptureProperty(pCapture, CV_CAP_PROP_POS_FRAMES);

            // Get total number of frames...
            int nTotalFrames = (int) 
                cvGetCaptureProperty(pCapture, CV_CAP_PROP_FRAME_COUNT);

            // We have the information we need to compute progress...
            if(nCurrentFrame && nTotalFrames)
            {
                // Compute total progress...
                int nProgress = 
                    (int)(((float) nCurrentFrame / nTotalFrames)  * 100.0);

                // Prepare current frame processing string...
                sTemp.Printf(wxT("%d / %d (%d %%)"), nCurrentFrame, 
                             nTotalFrames, nProgress);

                // Set it only if it has changed...
                if(nProgress != Frame.AnalysisGauge->GetValue())
                {
                    // Update the current frame...
                    Frame.AnalysisCurrentFrameStatus->ChangeValue(sTemp);

                    // Update the progress bar...
                    Frame.AnalysisGauge->SetValue(nProgress);
                }

                // The Quicktime backend appears to be buggy in that it keeps
                //  cycling through the video even after we have all frames.
                //  A temporary hack is to just break the analysis loop when we
                //  have both current frame, total frame, and they are equal...
                if(nCurrentFrame + 1 == nTotalFrames)
                    break;
            }
            
            // We cannot compute progress because the codec the backend on this
            //  platform is busted to shit...
            else
            {
                // To avoid jamming up the event pipeline, be infrequent...
                if(lLastPulseTime <= ::wxGetLocalTime() - 3)
                {
                    // Prepare current frame processing string...
                    sTemp.Printf(wxT("%d"), nCurrentFrame);

                    // Update the current frame...
                    Frame.AnalysisCurrentFrameStatus->ChangeValue(sTemp);

                    // Pulse progress bar...
                    Frame.AnalysisGauge->Pulse();
                    
                    // Update the last pulse time for next check...
                    lLastPulseTime = ::wxGetLocalTime();
                }
            }
    }

    // Show total time...
    sTemp.Printf(wxT("Took %.3f s..."), StopWatch.Time() / 1000.0f);
    Frame.AnalysisStatusList->Append(sTemp);

    // Flush the analysis frame queue...
    while(!Frame.AnalysisFrameBuffer.empty())
    {
        // Deallocate oldest...
        pThinkingImage = (IplImage *) Frame.AnalysisFrameBuffer.front();
        Frame.AnalysisFrameBuffer.pop_front();
        cvReleaseImage(&pThinkingImage);
    }

    // Release the capture source...
    cvReleaseCapture(&pCapture);

    // Done...
    return NULL;
}

