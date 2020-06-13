/*
  Name:         SlitherApp.h (definition)
  Author:       Kip Warner (Kip@TheVertigo.com)
  Description:  Every wxWidgets application defines an application class...
*/

// Multiple include protection...
#ifndef _SLITHERAPP_H_
#define _SLITHERAPP_H_

// To-do list...

// Includes...
    
    // wxWidgets...
    
        // If compiler supports header precompilation, use it...
        #include <wx/wxprec.h>

        // Otherwise, include the usual "standard" wxWidgets headers...
        #ifndef WX_PRECOMP
            #include <wx/wx.h>
        #endif

        // wxWidgets headers we need for this class...
        #include <wx/aboutdlg.h>
        #include <wx/config.h>
        #include <wx/filedlg.h>
        #include <wx/grid.h>
        #include <wx/image.h>
        #include <wx/imaglist.h>
        #include <wx/init.h>
        #include <wx/mediactrl.h>
        #include <wx/notebook.h>
        #include <wx/platinfo.h>
        #include <wx/progdlg.h>
        #include <wx/stdpaths.h>
        #include <wx/textdlg.h>
        #include <wx/tglbtn.h>
        #include <wx/tipdlg.h>
        #include <wx/txtstrm.h>
        #include <wx/wfstream.h>
        #include <wx/xml/xml.h>
        #include <wx/xrc/xmlres.h>
        #include <wx/zipstrm.h>

    // Resource stuff...

        /* If we're on Winblows, we need the manifest file for XP...
        #ifdef __WINDOWS__
            #include <wx/msw/wx.rc>
        #endif*/

        // Definition generated by wxrc and needed for GUI resources...
        extern void InitXmlResource();

        // Resource declarations...
        #include "Resources.h"


// Forward declarations...
class MainFrame;

// Slither application class...
class SlitherApp : public wxApp
{
    // Public stuff...
    public:
    
        // Methods...

            // Invoked on application startup... (analagous to main or WinMain)
            virtual bool OnInit();

            // Fatal exception handler...
            virtual void OnFatalException();
            
            // Exiting...
            virtual int  OnExit();
            
            // Mac specific stuff...
            #ifdef __WXMAC__
                
                // Mac open file request... (Finder doesn't use command line)
                virtual void MacOpenFile(const wxString &sFileName);

            #endif

        // Attributes...

            // Main frame...
            MainFrame          *pMainFrame;

            // Configuration object...
            wxConfig           *pConfiguration;
            
            // Standard system paths...
	    // $VR$: 2020/06/10 - deprecated usage, see CPP file
            //wxStandardPaths     StandardPaths;
            
            // Contains experiment to load if shell passed it to us...
            wxString            sExperimentRequestedFromShell;
};

    // Implements SlitherApp &wxGetApp()...
    DECLARE_APP(SlitherApp)

#endif