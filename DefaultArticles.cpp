#include "sm_ipedia.h"
#include "iPediaApplication.hpp"
#include <FormattedTextElement.hpp>
#include <FontEffects.hpp>
#include <LineBreakElement.hpp>
#include <ipedia.h>
#include <text.hpp>

using namespace ArsLexis;

void updateArticleCountEl(long articleCount, ArsLexis::String& dbTime)
{
    assert(NULL!=g_articleCountElement);
    assert(-1!=articleCount);
    assert(8==dbTime.length());
    char_t buffer[32];
    int len = formatNumber(articleCount, buffer, sizeof(buffer));
    assert(len != -1 );
    String articleCountText;
    articleCountText.append(buffer, len);
    articleCountText.append(_T(" articles, database updated on "));
    articleCountText.append(dbTime, 0, 4);
    articleCountText.append(1, '-');
    articleCountText.append(dbTime, 4, 2);
    articleCountText.append(1, '-');
    articleCountText.append(dbTime, 6, 2);
    g_articleCountElement->setText(articleCountText);
}


static void wikipediaActionCallback(void *data)
{
    assert(showWikipedia!=displayMode());
    setDisplayMode(showWikipedia);
    g_forceLayoutRecalculation = true;
    InvalidateRect(g_hwndMain, NULL, false);
}

static void tutorialActionCallback(void *data)
{
    assert(showTutorial!=displayMode());
    setDisplayMode(showTutorial);
    g_forceLayoutRecalculation = true;
    InvalidateRect(g_hwndMain, NULL, false);
}

static void unregisteredActionCallback(void *data)
{
    assert(showRegister!=displayMode());
    setDisplayMode(showRegister);
    g_forceLayoutRecalculation = true;
    InvalidateRect(g_hwndMain, NULL, false);
}

static void aboutActionCallback(void *data)
{
    assert(showAbout!=displayMode());
    setDisplayMode(showAbout);
    g_forceLayoutRecalculation = true;
    InvalidateRect(g_hwndMain, NULL, false);
}

static void randomArticleActionCallback(void *data)
{
    assert(showTutorial==displayMode());
    SendMessage(g_hwndMain, WM_COMMAND, IDM_MENU_RANDOM, 0);
}

void prepareAbout()
{
    int divider = 2;
#ifdef WIN32_PLATFORM_PSPC
    divider = 1;
#endif
    Definition::Elements_t elems;
    FormattedTextElement* text;

    FontEffects fxBold;
    fxBold.setWeight(FontEffects::weightBold);

    elems.push_back(new LineBreakElement(1,10));

    elems.push_back(text=new FormattedTextElement(_T("ArsLexis iPedia")));
    text->setJustification(DefinitionElement::justifyCenter);
    text->setStyle(styleHeader);
    text->setEffects(fxBold);

    elems.push_back(new LineBreakElement(1,3*divider));

    const char_t* version=_T("Ver ") appVersion
#ifdef INTERNAL_BUILD
    _T(" (internal)")
#endif
/*
#ifdef DEBUG
        _T(" (debug)")
#endif*/
    ;
    elems.push_back(text=new FormattedTextElement(version));
    text->setJustification(DefinitionElement::justifyCenter);
    elems.push_back(new LineBreakElement(1,4*divider));

    iPediaApplication& app=iPediaApplication::instance();
    if (app.preferences().regCode.empty())
    {
        elems.push_back(text=new FormattedTextElement(_T("Unregistered (")));
        text->setJustification(DefinitionElement::justifyCenter);
        elems.push_back(text=new FormattedTextElement(_T("how to register")));
        text->setJustification(DefinitionElement::justifyCenter);
        // url doesn_T('t really matter, it')s only to establish a hotspot
        text->setHyperlink(_T(""), hyperlinkTerm);
        text->setActionCallback( unregisteredActionCallback, NULL);
        elems.push_back(text=new FormattedTextElement(_T(")")));
        text->setJustification(DefinitionElement::justifyCenter);
        elems.push_back(new LineBreakElement(1,2*divider));
    }
    else
    {
        elems.push_back(text=new FormattedTextElement(_T("Registered")));
        text->setJustification(DefinitionElement::justifyCenter);
        elems.push_back(new LineBreakElement(1,2*divider));
    }

    elems.push_back(text=new FormattedTextElement(_T("Software \251 ")));
    text->setJustification(DefinitionElement::justifyCenter);

    elems.push_back(text=new FormattedTextElement(_T("ArsLexis")));
    text->setJustification(DefinitionElement::justifyCenter);
    text->setHyperlink(_T("http://www.arslexis.com/pda/palm.html"), hyperlinkExternal);

    elems.push_back(new LineBreakElement(1,4*divider));
    elems.push_back(text=new FormattedTextElement(_T("Data \251 ")));
    text->setJustification(DefinitionElement::justifyCenter);

    elems.push_back(text=new FormattedTextElement(_T("WikiPedia")));
    text->setJustification(DefinitionElement::justifyCenter);
    // url doesn_T('t really matter, it')s only to establish a hotspot
    text->setHyperlink(_T(""), hyperlinkTerm);
    text->setActionCallback( wikipediaActionCallback, NULL);

    elems.push_back(new LineBreakElement(1,1*divider));
    elems.push_back(g_articleCountElement=new FormattedTextElement(_T(" ")));
    if (-1!=g_articleCountSet)
    {
        iPediaApplication& app=iPediaApplication::instance();
        updateArticleCountEl(g_articleCountSet,app.preferences().databaseTime);
    }
    g_articleCountElement->setJustification(DefinitionElement::justifyCenter);

    elems.push_back(new LineBreakElement(3,2*divider));
    elems.push_back(text=new FormattedTextElement(_T("Using iPedia: ")));
    text->setJustification(DefinitionElement::justifyLeft);

    elems.push_back(text=new FormattedTextElement(_T("tutorial")));
    text->setJustification(DefinitionElement::justifyLeft);
    // url doesn_T('t really matter, it')s only to establish a hotspot
    text->setHyperlink(_T(""), hyperlinkTerm);
    text->setActionCallback( tutorialActionCallback, NULL);

    (*g_about).replaceElements(elems);    
}

// TODO: make those on-demand only to save memory
void prepareTutorial()
{
    Definition::Elements_t elems;
    FormattedTextElement* text;

    assert( (*g_tutorial).empty() );

    FontEffects fxBold;
    fxBold.setWeight(FontEffects::weightBold);

    elems.push_back(text=new FormattedTextElement(_T("Go back to main screen.")));
    text->setJustification(DefinitionElement::justifyLeft);
    // url doesn_T('t really matter, it')s only to establish a hotspot
    text->setHyperlink(_T(""), hyperlinkTerm);
    text->setActionCallback( aboutActionCallback, NULL);
    elems.push_back(new LineBreakElement(4,3));

    elems.push_back(text=new FormattedTextElement(_T("iPedia is a wireless encyclopedia. Use it to get information and facts on just about anything.")));
    elems.push_back(new LineBreakElement(4,3));

    elems.push_back(text=new FormattedTextElement(_T("Finding an encyclopedia article.")));
    text->setEffects(fxBold);
    elems.push_back(text=new FormattedTextElement(_T(" Let's assume you want to read an encyclopedia article on Seattle. Enter 'Seattle' in the text field at the bottom of the screen and press 'Search' (or center button on Treo's 5-way navigator).")));
    text->setJustification(DefinitionElement::justifyLeft);
    elems.push_back(new LineBreakElement(4,3));

    elems.push_back(text=new FormattedTextElement(_T("Finding all articles with a given word.")));
    text->setEffects(fxBold);
    elems.push_back(text=new FormattedTextElement(_T(" Let's assume you want to find all articles that mention Seattle. Enter 'Seattle' in the text field and use 'Main/Extended search' menu item. In response you'll receive a list of articles that contain word 'Seattle'.")));
    text->setJustification(DefinitionElement::justifyLeft);
    elems.push_back(new LineBreakElement(4,3));

    elems.push_back(text=new FormattedTextElement(_T("Refining the search.")));
    text->setEffects(fxBold);
    elems.push_back(text=new FormattedTextElement(_T(" If there are too many results, you can refine (narrow) the search results by adding additional terms e.g. type 'museum' and press 'Refine' button. You'll get a smaller list of articles that contain both 'Seattle' and 'museum'.")));
    text->setJustification(DefinitionElement::justifyLeft);
    elems.push_back(new LineBreakElement(4,3));

    elems.push_back(text=new FormattedTextElement(_T("Results of last extended search.")));
    text->setEffects(fxBold);
    elems.push_back(text=new FormattedTextElement(_T(" At any time you can get a list of results from last extended search by using menu item 'Main/Extended search results'.")));
    text->setJustification(DefinitionElement::justifyLeft);
    elems.push_back(new LineBreakElement(4,3));

    elems.push_back(text=new FormattedTextElement(_T("Random article.")));
    text->setEffects(fxBold);
    elems.push_back(text=new FormattedTextElement(_T(" You can use menu 'Main/Random article' (or ")));
    text->setJustification(DefinitionElement::justifyLeft);
    elems.push_back(text=new FormattedTextElement(_T("click here")));
    text->setJustification(DefinitionElement::justifyLeft);
    // url doesn_T('t really matter, it')s only to establish a hotspot
    text->setHyperlink(_T(""), hyperlinkTerm);
    text->setActionCallback( randomArticleActionCallback, NULL);
    elems.push_back(text=new FormattedTextElement(_T(") to get a random article.")));
    text->setJustification(DefinitionElement::justifyLeft);
    elems.push_back(new LineBreakElement(4,3));

    elems.push_back(text=new FormattedTextElement(_T("More information.")));
    text->setEffects(fxBold);
    elems.push_back(text=new FormattedTextElement(_T(" Please visit our website ")));
    text->setJustification(DefinitionElement::justifyLeft);

    elems.push_back(text=new FormattedTextElement(_T("arslexis.com")));
    text->setHyperlink(_T("http://www.arslexis.com/pda/palm.html"), hyperlinkExternal);
    text->setJustification(DefinitionElement::justifyLeft);

    elems.push_back(text=new FormattedTextElement(_T(" for more information about iPedia.")));
    text->setJustification(DefinitionElement::justifyLeft);
    elems.push_back(new LineBreakElement(4,3));

    elems.push_back(text=new FormattedTextElement(_T("Go back to main screen.")));
    text->setJustification(DefinitionElement::justifyLeft);
    // url doesn_T('t really matter, it')s only to establish a hotspot
    text->setHyperlink(_T(""), hyperlinkTerm);
    text->setActionCallback( aboutActionCallback, NULL );

    (*g_tutorial).replaceElements(elems);
}

static void registerActionCallback(void *data)
{
    assert(NULL!=data);
    SendMessage(g_hwndMain, WM_COMMAND, IDM_MENU_REGISTER, 0);
}

// TODO: make those on-demand only to save memory
void prepareHowToRegister()
{
    Definition::Elements_t elems;
    FormattedTextElement* text;

    assert( (*g_register).empty() );

    FontEffects fxBold;
    fxBold.setWeight(FontEffects::weightBold);

    elems.push_back(text=new FormattedTextElement(_T("Unregistered version of iPedia limits how many articles can be viewed in one day (there are no limits on random articles.)")));
    elems.push_back(new LineBreakElement());

    elems.push_back(text=new FormattedTextElement(_T("In order to register iPedia you need to purchase registration code at ")));

// those 3 #defines should be mutually exclusive
#ifdef PALMGEAR
    elems.push_back(text=new FormattedTextElement(_T("palmgear.com?67708")));
#endif

#ifdef HANDANGO
    elems.push_back(text=new FormattedTextElement(_T("handango.com/purchase, product id: 128991")));
#endif

#ifdef ARSLEXIS_VERSION
    elems.push_back(text=new FormattedTextElement(_T("our website ")));
    elems.push_back(text=new FormattedTextElement(_T("http://www.arslexis.com")));
    text->setHyperlink(_T("http://www.arslexis.com/pda/palm.html"), hyperlinkExternal);
#endif
    elems.push_back(new LineBreakElement());

    elems.push_back(text=new FormattedTextElement(_T("After obtaining registration code use menu item 'Options/Register' (or ")));
    elems.push_back(text=new FormattedTextElement(_T("click here")));
    // url doesn_T('t really matter, it')s only to establish a hotspot
    text->setHyperlink(_T(""), hyperlinkTerm);
    text->setActionCallback( registerActionCallback, NULL );
    elems.push_back(text=new FormattedTextElement(_T(") to enter registration code. ")));

    elems.push_back(text=new FormattedTextElement(_T("Go back to main screen.")));
    text->setJustification(DefinitionElement::justifyLeft);
    // url doesn_T('t really matter, it')s only to establish a hotspot
    text->setHyperlink(_T(""), hyperlinkTerm);
    text->setActionCallback( aboutActionCallback, NULL );

    (*g_register).replaceElements(elems);
}

void prepareWikipedia()
{
    Definition::Elements_t elems;
    FormattedTextElement* text;

    assert( (*g_wikipedia).empty() );

    FontEffects fxBold;
    fxBold.setWeight(FontEffects::weightBold);

    elems.push_back(text=new FormattedTextElement(_T("All the articles in iPedia come from WikiPedia project and are licensed under ")));
    elems.push_back(text=new FormattedTextElement(_T("GNU Free Documentation License")));
    text->setHyperlink(_T("http://www.gnu.org/copyleft/fdl.html"), hyperlinkExternal);
    elems.push_back(text=new FormattedTextElement(_T(".")));
    elems.push_back(new LineBreakElement());

    elems.push_back(text=new FormattedTextElement(_T("To find out more about WikiPedia project, visit ")));
    elems.push_back(text=new FormattedTextElement(_T("wikipedia.org")));
    text->setHyperlink(_T("http://www.wikipedia.org"), hyperlinkExternal);
    elems.push_back(text=new FormattedTextElement(_T(" website. ")));
    elems.push_back(new LineBreakElement());

    elems.push_back(text=new FormattedTextElement(_T("Go back to main screen.")));
    text->setJustification(DefinitionElement::justifyLeft);
    // url doesn_T('t really matter, it')s only to establish a hotspot
    text->setHyperlink(_T(""), hyperlinkTerm);
    text->setActionCallback( aboutActionCallback, NULL );

    (*g_wikipedia).replaceElements(elems);
}    
