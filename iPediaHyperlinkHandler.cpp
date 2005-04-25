#include "sm_ipedia.h"
#include <WinSysUtils.hpp>
#include <LookupManager.hpp>
#include <TextElement.hpp>
#include "iPediaHyperlinkHandler.hpp"
#include "iPediaApplication.hpp"
#include <LangNames.hpp>

/*
bool iPediaHyperlinkHandler::handleExternalHyperlink(const ArsLexis::String& url)
{
    return GotoURL(url.c_str());
    
}

bool iPediaHyperlinkHandler::handleTermHyperlink(const ArsLexis::String& term)
{
    iPediaApplication& app=GetApp();
    if (app.fLookupInProgress())
        return false;

    LookupManager* lookupManager=app.lookupManager;  
    lookupManager->lookupIfDifferent(term);
    SetUIState(false);
    return true;
}
*/

iPediaHyperlinkHandler::iPediaHyperlinkHandler()
{
}

/*
void iPediaHyperlinkHandler::handleHyperlink(Definition& definition, DefinitionElement& element)
{
    assert(element.isTextElement());
    TextElement& textElement=static_cast<TextElement&>(element);
    assert(textElement.isHyperlink());
    const TextElement::HyperlinkProperties* props=textElement.hyperlinkProperties();
    assert(props!=0);
    bool makeClicked=false;
    switch (props->type) 
    {
        //case hyperlinkBookmark:
        //    definition.goToBookmark(props->resource);
        //    break;
    
        case hyperlinkExternal:
            makeClicked=handleExternalHyperlink(props->resource);
            break;
            
        case hyperlinkTerm:            
            makeClicked=handleTermHyperlink(props->resource);
            break;

        default:
            assert(false);
       
    }  
}
*/

bool iPediaHyperlinkHandler::handleTermHyperlink(const char_t* term, ulong_t len, const Point*)
{
    iPediaApplication& app=GetApp();
    if (app.fLookupInProgress())
        return false;

    LookupManager* lookupManager=app.lookupManager;  
	String lang;
	long pos = StrFind(term, len, _T(':'));
	if (-1 != pos)
	{
		const char_t* langName = GetLangNameByLangCode(term, pos);
		if (NULL != langName)
		{
			lang.assign(term, pos);
			term += (pos + 1);
			len -= (pos + 1);
		}
	}
    lookupManager->lookupIfDifferent(term, lang);
    SetUIState(false);
    return true;
}


void iPediaHyperlinkHandler::handleHyperlink(const char_t* link, ulong_t len, const Point* p)
{
	if (StrStartsWith(link, len, _T("http://"), -1))
	{
		GotoURL(link);
	}
	else
		handleTermHyperlink(link, len, p);
}
