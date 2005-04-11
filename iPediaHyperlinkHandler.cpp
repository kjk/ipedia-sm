#include "sm_ipedia.h"
#include <WinSysUtils.hpp>
#include <LookupManager.hpp>
#include <TextElement.hpp>
#include "iPediaHyperlinkHandler.hpp"
#include "iPediaApplication.hpp"

bool iPediaHyperlinkHandler::handleExternalHyperlink(const ArsLexis::String& url)
{
    return GotoURL(url.c_str());
    
}

bool iPediaHyperlinkHandler::handleTermHyperlink(const ArsLexis::String& term)
{
    iPediaApplication& app=GetApp();
    if (app.fLookupInProgress())
        return false;

    LookupManager* lookupManager=app.getLookupManager();  
    lookupManager->lookupIfDifferent(term);
    SetUIState(false);
    return true;
}

iPediaHyperlinkHandler::iPediaHyperlinkHandler()
{
}

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

