#ifndef __IPEDIA_HYPERLINK_HANDLER_HPP__
#define __IPEDIA_HYPERLINK_HANDLER_HPP__

#include "Definition.hpp"

class iPediaHyperlinkHandler: public Definition::HyperlinkHandler
{
    
    bool handleExternalHyperlink(const ArsLexis::String& url);
    bool handleTermHyperlink(const ArsLexis::String& term);
    
public:

    iPediaHyperlinkHandler();
    
    void handleHyperlink(Definition& definition, DefinitionElement& hyperlinkElement);
    
};

#endif