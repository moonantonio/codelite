#include "findusagetab.h"
#include "findresultstab.h"
#include <wx/xrc/xmlres.h>
#include <wx/ffile.h>
#include <wx/tokenzr.h>
#include "ctags_manager.h"
#include "cl_editor.h"
#include "frame.h"
#include "editor_config.h"

FindUsageTab::FindUsageTab(wxWindow* parent, const wxString &name)
    : OutputTabWindow(parent, wxID_ANY, name)
{
    FindResultsTab::SetStyles(m_sci);
    m_sci->Connect(wxEVT_STC_STYLENEEDED, wxStyledTextEventHandler(FindUsageTab::OnStyleNeeded), NULL, this);
    m_tb->RemoveTool ( XRCID ( "repeat_output" ) );
    m_tb->Realize();
}

FindUsageTab::~FindUsageTab()
{
}

void FindUsageTab::OnStyleNeeded(wxStyledTextEvent& e)
{
    wxStyledTextCtrl* ctrl = dynamic_cast<wxStyledTextCtrl*> (e.GetEventObject());
    if ( !ctrl )
        return;
    FindResultsTab::StyleText(ctrl, e);
}

void FindUsageTab::Clear()
{
    m_matches.clear();
    OutputTabWindow::Clear();
}

void FindUsageTab::OnClearAll(wxCommandEvent& e)
{
    Clear();
}

void FindUsageTab::OnMouseDClick(wxStyledTextEvent& e)
{
    long pos = e.GetPosition();
    int line = m_sci->LineFromPosition(pos);
    UsageResultsMap::const_iterator iter = m_matches.find(line);
    if (iter != m_matches.end()) {
        DoOpenResult( iter->second );
    }

    m_sci->SetSelection(wxNOT_FOUND, pos);
}

void FindUsageTab::OnClearAllUI(wxUpdateUIEvent& e)
{
    e.Enable(m_sci && m_sci->GetLength());
}

void FindUsageTab::ShowUsage(const std::list<CppToken>& matches, const wxString& searchWhat)
{
    Clear();
    int           lineNumber(0);
    wxString      text;
    wxString      curfile;
    wxString      curfileContent;
    wxArrayString lines;

    text = wxString::Format(_("===== Finding references of '%s' =====\n"), searchWhat.c_str());
    lineNumber++;

    std::list<CppToken>::const_iterator iter = matches.begin();
    for(; iter != matches.end(); iter++) {

        // Print the line number
        wxString file_name(iter->getFilename().c_str(), wxConvUTF8);
        if(curfile != file_name) {
            curfile = file_name;
            wxFileName fn(file_name);
            fn.MakeRelativeTo();

            text << fn.GetFullPath() << wxT("\n");
            lineNumber++;

            // Load the file content
            wxLogNull nolog;
            wxFFile thefile(file_name, wxT("rb"));
            if(thefile.IsOpened()) {

                wxFileOffset size = thefile.Length();
                wxString fileData;
                fileData.Alloc(size);
                curfileContent.Clear();

                wxCSConv fontEncConv(wxFONTENCODING_ISO8859_1);
                thefile.ReadAll( &curfileContent, fontEncConv );

                // break the current file into lines, a line can be an empty string
                lines = wxStringTokenize(curfileContent, wxT("\n"), wxTOKEN_RET_EMPTY_ALL);
            }

        }

        // Keep the match
        m_matches[lineNumber] = *iter;

        // Format the message
        wxString linenum = wxString::Format(wxT(" %5u "), (unsigned int)iter->getLineNumber() + 1);
        wxString scopeName (wxT("<global>"));
        TagEntryPtr tag = TagsManagerST::Get()->FunctionFromFileLine(wxString(iter->getFilename().c_str(), wxConvUTF8), iter->getLineNumber());
        if(tag) {
            scopeName = tag->GetPath();
        }

        text << linenum << wxT("[ ") << scopeName << wxT(" ] ");
        if(lines.GetCount() > iter->getLineNumber()) {
            text << lines.Item(iter->getLineNumber()).Trim().Trim(false);
        }

        text << wxT("\n");
        lineNumber++;
    }
    text << wxString::Format(_("===== Found total of %u matches =====\n"), (unsigned int)m_matches.size());
    AppendText( text );
}

void FindUsageTab::DoOpenResult(const CppToken& token)
{
    if (!token.getFilename().empty()) {
        LEditor *editor = clMainFrame::Get()->GetMainBook()->OpenFile(wxString(token.getFilename().c_str(), wxConvUTF8), wxEmptyString, token.getLineNumber());
        if(editor) {
            editor->GotoLine(token.getLineNumber());
            editor->ScrollToColumn(0);
            editor->SetSelection(token.getOffset(), token.getOffset() + token.getName().length());
        }
    }
}

void FindUsageTab::OnHoldOpenUpdateUI(wxUpdateUIEvent& e)
{
    if(EditorConfigST::Get()->GetOptions()->GetHideOutpuPaneOnUserClick()) {
        e.Enable(true);
        e.Check( EditorConfigST::Get()->GetOptions()->GetHideOutputPaneNotIfReferences() );

    } else {
        e.Enable(false);
        e.Check(false);
    }
}
