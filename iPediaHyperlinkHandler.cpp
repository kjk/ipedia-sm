#include "sm_ipedia.h"
#include "iPediaHyperlinkHandler.hpp"
#include "iPediaApplication.hpp"
#include "LookupManager.hpp"
#include "GenericTextElement.hpp"

bool iPediaHyperlinkHandler::handleExternalHyperlink(const ArsLexis::String& url)
{
    return GotoURL(url.c_str());
    
}

bool iPediaHyperlinkHandler::handleTermHyperlink(const ArsLexis::String& term)
{
    bool result=false;
    iPediaApplication& app=iPediaApplication::instance();
    LookupManager* lookupManager=app.getLookupManager();  
    if (lookupManager && !lookupManager->lookupInProgress())
    {
        result=true;
        lookupManager->lookupIfDifferent(term);
        setUIState(false);
    }
    return result;        
}


iPediaHyperlinkHandler::iPediaHyperlinkHandler()
{
}

void iPediaHyperlinkHandler::handleHyperlink(Definition& definition, DefinitionElement& element)
{
    assert(element.isTextElement());
    GenericTextElement& textElement=static_cast<GenericTextElement&>(element);
    assert(textElement.isHyperlink());
    const GenericTextElement::HyperlinkProperties* props=textElement.hyperlinkProperties();
    assert(props!=0);
    bool makeClicked=false;
    switch (props->type) 
    {
        case hyperlinkBookmark:
            definition.goToBookmark(props->resource);
            break;
    
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

/*  if (makeClicked) 
    {
        iPediaApplication& app=iPediaApplication::instance();
        props->type=hyperlinkClicked;
        ArsLexis::Form* form=app.getOpenForm(mainForm);
        if (form) 
        {
            ArsLexis::Graphics graphics(form->windowHandle());
            definition.renderSingleElement(graphics, app.preferences().renderingPreferences, element);
        }
    }
*/    
