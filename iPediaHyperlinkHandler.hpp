#ifndef __IPEDIA_HYPERLINK_HANDLER_HPP__
#define __IPEDIA_HYPERLINK_HANDLER_HPP__

#include "Definition.hpp"

class iPediaHyperlinkHandler: public Definition::HyperlinkHandler
{
    
//    bool handleExternalHyperlink(const char_t* link, ulong_t len, const Point*);
    bool handleTermHyperlink(const char_t* link, ulong_t len, const Point*);
    
public:

    iPediaHyperlinkHandler();
    
	void handleHyperlink(const char_t* link, ulong_t len, const Point*);
};

#endif