#include <TextElement.hpp>
#include <ParagraphElement.hpp>
#include <FontEffects.hpp>
#include <LineBreakElement.hpp>
#include <text.hpp>
#include <ipedia.h>
#include <LangNames.hpp>

#include "iPediaApplication.hpp"
#include "sm_ipedia.h"
#include "iPediaStyles.hpp"

TextElement* g_articleCountElement = NULL;

void updateArticleCountEl(long articleCount, ArsLexis::String& dbTime)
{
    assert(NULL != g_articleCountElement);
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
    SetDisplayMode(showWikipedia);
    g_forceLayoutRecalculation = true;
    InvalidateRect(iPediaApplication::instance().getMainWindow(), NULL, false);
}

static void tutorialActionCallback(void *data)
{
    assert(showTutorial!=displayMode());
    SetDisplayMode(showTutorial);
    g_forceLayoutRecalculation = true;
    InvalidateRect(iPediaApplication::instance().getMainWindow(), NULL, false);
}

static void unregisteredActionCallback(void *data)
{
    assert(showRegister!=displayMode());
    SetDisplayMode(showRegister);
    g_forceLayoutRecalculation = true;
    InvalidateRect(iPediaApplication::instance().getMainWindow(), NULL, false);
}

static void aboutActionCallback(void *data)
{
    assert(showAbout!=displayMode());
    SetDisplayMode(showAbout);
    g_forceLayoutRecalculation = true;
    InvalidateRect(iPediaApplication::instance().getMainWindow(), NULL, false);
}

static void randomArticleActionCallback(void *data)
{
    assert(showTutorial==displayMode());
    SendMessage(iPediaApplication::instance().getMainWindow(), WM_COMMAND, IDM_MENU_RANDOM, 0);
}

static void prepareArticleCountEl(TextElement *articleCountElement, long articleCount, const ArsLexis::String& dbTime)
{
    assert(NULL!=articleCountElement);

    assert(-1 != articleCount);
    assert(8 == dbTime.length());
    char_t buffer[32];
    int len = formatNumber(articleCount, buffer, sizeof(buffer));
    assert(len != -1 );
    String articleCountText;
    articleCountText.append(buffer, len);
    articleCountText.append(_T(" articles. "));

    iPediaApplication& app = GetApp();
    const char_t* langName = GetLangNameByLangCode(app.preferences().currentLang.data(), app.preferences().currentLang.length());
    if (NULL == langName)
        langName = _T("Unknown");
        
    articleCountText.append(langName);

    articleCountText.append(_T(" encyclopedia last updated on "));
    articleCountText.append(dbTime, 0, 4);
    articleCountText.append(1, _T('-'));
    articleCountText.append(dbTime, 4, 2);
    articleCountText.append(1, _T('-'));
    articleCountText.append(dbTime, 6, 2);
    articleCountElement->setText(articleCountText);

}

void prepareAbout(Definition *def)
{
	DefinitionModel* model = new DefinitionModel();

    int divider = 2;
#ifdef WIN32_PLATFORM_PSPC
    divider = 1;
#endif
    Definition::Elements_t& elems = model->elements;
    TextElement* text;
    FontEffects fxBold;
    fxBold.setWeight(FontEffects::weightBold);

#ifdef WIN32_PLATFORM_PSPC
    elems.push_back(new LineBreakElement(1,1));
#else
    elems.push_back(new LineBreakElement(1,10));
#endif

    elems.push_back(text=new TextElement(_T("ArsLexis iPedia")));
    text->setJustification(DefinitionElement::justifyCenter);
    text->setStyle(StyleGetStaticStyle(styleNameHeader));

    elems.push_back(new LineBreakElement(1,3*divider*divider));

#ifdef WIN32_PLATFORM_PSPC
    const char_t* version=_T("Ver 1.2")
#else
    const char_t* version=_T("Ver 1.2")
#endif

#ifdef DEBUG
        _T(" (debug)")
#endif
    ;
    elems.push_back(text=new TextElement(version));
    text->setJustification(DefinitionElement::justifyCenter);

    elems.push_back(new LineBreakElement(1,4*divider));
    iPediaApplication& app = GetApp();
    if (app.preferences().regCode.empty())
    {
#ifdef WIN32_PLATFORM_PSPC
        elems.push_back(text=new TextElement(_T("Unregistered (")));
        text->setJustification(DefinitionElement::justifyCenter);
        elems.push_back(text=new TextElement(_T("how to register")));
        text->setJustification(DefinitionElement::justifyCenter);
        text->setHyperlink(_T("How to register"), hyperlinkTerm);
        text->setActionCallback( unregisteredActionCallback, NULL);
        elems.push_back(text=new TextElement(_T(")")));
        text->setJustification(DefinitionElement::justifyCenter);
        elems.push_back(new LineBreakElement(1,4*divider));
#else
        elems.push_back(text=new TextElement(_T("Unregistered")));
        text->setJustification(DefinitionElement::justifyCenter);
        elems.push_back(new LineBreakElement(1,4*divider));
#endif
    }
    else
    {
        elems.push_back(text=new TextElement(_T("Registered")));
        text->setJustification(DefinitionElement::justifyCenter);
        elems.push_back(new LineBreakElement(1,4*divider));
    }

    elems.push_back(text=new TextElement(_T("Software \251 ")));
    text->setJustification(DefinitionElement::justifyCenter);

    elems.push_back(text=new TextElement(_T("ArsLexis")));
    text->setJustification(DefinitionElement::justifyCenter);

#ifdef WIN32_PLATFORM_PSPC
    text->setHyperlink(_T("http://www.arslexis.com/pda/ppc.html"), hyperlinkExternal);
#else
    text->setHyperlink(_T("http://www.arslexis.com/pda/sm.html"), hyperlinkExternal);
#endif

    elems.push_back(new LineBreakElement(1,4*divider));
    elems.push_back(text=new TextElement(_T("Data \251 ")));
    text->setJustification(DefinitionElement::justifyCenter);

    elems.push_back(text=new TextElement(_T("WikiPedia")));
    text->setJustification(DefinitionElement::justifyCenter);
    text->setHyperlink(_T("WikiPedia"), hyperlinkTerm);
    text->setActionCallback( wikipediaActionCallback, NULL);

    elems.push_back(new LineBreakElement(1,2*divider));

    g_articleCountElement=new TextElement(_T(" "));

    if (-1 != app.preferences().articleCount)
    {
        prepareArticleCountEl(g_articleCountElement, app.preferences().articleCount, app.preferences().databaseTime);
    }

    elems.push_back(g_articleCountElement);
    g_articleCountElement->setJustification(DefinitionElement::justifyCenter);

#ifdef WIN32_PLATFORM_PSPC
    elems.push_back(new LineBreakElement(3,divider));
    elems.push_back(text=new TextElement(_T("Using iPedia: ")));
    text->setJustification(DefinitionElement::justifyLeft);

    elems.push_back(text=new TextElement(_T("tutorial")));
    text->setJustification(DefinitionElement::justifyLeft);
    text->setHyperlink(_T("Tutorial"), hyperlinkTerm);
    text->setActionCallback( tutorialActionCallback, NULL);
#else
    elems.push_back(new LineBreakElement(1,2));
    elems.push_back(text=new TextElement(_T("Downloading uses data connection")));
    text->setJustification(DefinitionElement::justifyCenter);
	DefinitionStyle* style = new DefinitionStyle();
	style->small = style->yes;
	text->setStyle(style, DefinitionElement::ownStyle);

#endif

    def->setModel(model, Definition::ownModel);
}

// TODO: make those on-demand only to save memory
void prepareTutorial(Definition *def)
{
	DefinitionModel* model = new DefinitionModel();
    Definition::Elements_t& elems = model->elements;
    TextElement* text;

    assert( def->empty() );
    
    ParagraphElement* parent=0; 
    
    elems.push_back(parent=new ParagraphElement());    
    FontEffects fxBold;
    fxBold.setWeight(FontEffects::weightBold);

    elems.push_back(text=new TextElement(_T("Home.")));

    text->setJustification(DefinitionElement::justifyLeft);
    text->setParent(parent);
    text->setHyperlink(_T("Main screen"), hyperlinkTerm);
    text->setActionCallback( aboutActionCallback, NULL);
    elems.push_back(new LineBreakElement(2,3));

    elems.push_back(parent=new ParagraphElement());    
    elems.push_back(text=new TextElement(_T("iPedia is a wireless encyclopedia. Use it to get information and facts on just about anything.")));
    text->setJustification(DefinitionElement::justifyLeft);
    text->setParent(parent);    
    elems.push_back(new LineBreakElement(2,3));
    
    elems.push_back(parent=new ParagraphElement());    
    elems.push_back(text=new TextElement(_T("Finding an encyclopedia article.")));
    text->setJustification(DefinitionElement::justifyLeft);
	text->setStyle(StyleGetStaticStyle(styleNameBold));
    text->setParent(parent);    
#ifdef WIN32_PLATFORM_PSPC
    text = new TextElement(_T(" Let's assume you want to read an encyclopedia article on Seattle. Enter 'Seattle' in the text field at the top of the screen and press 'Search'"));
    elems.push_back(text);
#else
    text = new TextElement(_T(" Let's assume you want to read an encyclopedia article on Seattle. Enter 'Seattle' in the text field at the top of the screen and press 'Go'"));
    elems.push_back(text);
#endif
    text->setJustification(DefinitionElement::justifyLeft);
    text->setParent(parent);    
    elems.push_back(new LineBreakElement(2,3));

    elems.push_back(parent=new ParagraphElement());    
    elems.push_back(text=new TextElement(_T("Finding all articles with a given word.")));
	text->setStyle(StyleGetStaticStyle(styleNameBold));
    text->setParent(parent);    

    elems.push_back(text=new TextElement(_T(" Let's assume you want to find all articles that mention Seattle. Enter 'Seattle' in the text field and use '")  MAIN_MENU_BUTTON _T("/Extended search' menu item. In response you'll receive a list of articles that contain word 'Seattle'.")));
    text->setJustification(DefinitionElement::justifyLeft);
    text->setParent(parent);    
    elems.push_back(new LineBreakElement(2,3));
    
    elems.push_back(parent=new ParagraphElement());    
    elems.push_back(text=new TextElement(_T("Refining the search.")));
	text->setStyle(StyleGetStaticStyle(styleNameBold));
    text->setParent(parent);    
    elems.push_back(text=new TextElement(_T(" If there are too many results, you can refine (narrow) the search results by adding additional terms e.g. type 'museum' and press 'Refine' button. You'll get a smaller list of articles that contain both 'Seattle' and 'museum'.")));
    text->setJustification(DefinitionElement::justifyLeft);
    text->setParent(parent);    
    elems.push_back(new LineBreakElement(2,3));

    elems.push_back(parent=new ParagraphElement());    
    elems.push_back(text=new TextElement(_T("Results of last extended search.")));
	text->setStyle(StyleGetStaticStyle(styleNameBold));
    text->setParent(parent);    
    elems.push_back(text=new TextElement(_T(" At any time you can get a list of results from last extended search by using menu item '") MAIN_MENU_BUTTON _T("/Extended search results'.")));
    text->setJustification(DefinitionElement::justifyLeft);
    text->setParent(parent);    
    elems.push_back(new LineBreakElement(2,3));

    elems.push_back(parent=new ParagraphElement());    
    elems.push_back(text=new TextElement(_T("Random article.")));
    text->setParent(parent);    
	text->setStyle(StyleGetStaticStyle(styleNameBold));
#ifdef WIN32_PLATFORM_PSPC
    elems.push_back(text=new TextElement(_T(" You can use menu 'Main/Random article' (or ")));
    text->setJustification(DefinitionElement::justifyLeft);
    text->setParent(parent);    
    elems.push_back(text=new TextElement(_T("click here")));
    text->setJustification(DefinitionElement::justifyLeft);
    text->setParent(parent);    
    text->setHyperlink(_T("Random article"), hyperlinkTerm);
    text->setActionCallback( randomArticleActionCallback, NULL);
    elems.push_back(text=new TextElement(_T(") to get a random article.")));
    text->setParent(parent);    
    text->setJustification(DefinitionElement::justifyLeft);
#else
    elems.push_back(text=new TextElement(_T(" Use menu 'Menu/Random article'")));
    text->setJustification(DefinitionElement::justifyLeft);
    text->setParent(parent);    
    elems.push_back(text=new TextElement(_T(" to get a random article.")));
    text->setJustification(DefinitionElement::justifyLeft);
    text->setParent(parent);    
#endif

    elems.push_back(new LineBreakElement(2,3));

    elems.push_back(parent=new ParagraphElement());    
    elems.push_back(text=new TextElement(_T("More information.")));
	text->setStyle(StyleGetStaticStyle(styleNameBold));
    text->setParent(parent);    
    elems.push_back(text=new TextElement(_T(" Please visit our website ")));
    text->setJustification(DefinitionElement::justifyLeft);
    text->setParent(parent);    

    elems.push_back(text=new TextElement(_T("arslexis.com")));
#ifdef WIN32_PLATFORM_PSPC
    text->setHyperlink(_T("http://www.arslexis.com/pda/ppc.html"), hyperlinkExternal);
#else
    text->setHyperlink(_T("http://www.arslexis.com/pda/sm.html"), hyperlinkExternal);
#endif
    text->setJustification(DefinitionElement::justifyLeft);
    text->setParent(parent);    

    elems.push_back(text=new TextElement(_T(" for more information about iPedia.")));
    text->setJustification(DefinitionElement::justifyLeft);
    text->setParent(parent);    
    elems.push_back(new LineBreakElement(2,3));

    elems.push_back(parent=new ParagraphElement());    

    elems.push_back(text=new TextElement(_T("Home.")));

    text->setJustification(DefinitionElement::justifyLeft);
    text->setParent(parent);    
    text->setHyperlink(_T("Main screen"), hyperlinkTerm);
    text->setActionCallback( aboutActionCallback, NULL );

    def->setModel(model, Definition::ownModel);
}

static void registerActionCallback(void *data)
{
    // assert(NULL != data);
    SendMessage(iPediaApplication::instance().getMainWindow(), WM_COMMAND, IDM_MENU_REGISTER, 0);
}

// TODO: make those on-demand only, to save memory
void prepareHowToRegister(Definition *def)
{
	DefinitionModel* model = new DefinitionModel();
    Definition::Elements_t& elems = model->elements;
    TextElement* text;

    assert( def->empty() );

    FontEffects fxBold;
    fxBold.setWeight(FontEffects::weightBold);

    elems.push_back(text=new TextElement(_T("Home.")));
    text->setJustification(DefinitionElement::justifyLeft);
    text->setHyperlink(_T("Main screen"), hyperlinkTerm);
    text->setActionCallback( aboutActionCallback, NULL);
    elems.push_back(new LineBreakElement(2,3));

    elems.push_back(text=new TextElement(_T("Unregistered version of iPedia limits how many articles can be viewed in one day (there are no limits on random articles.)")));

    elems.push_back(new LineBreakElement());

    elems.push_back(text=new TextElement(_T("In order to register iPedia you need to purchase registration code at ")));

// those 3 #defines should be mutually exclusive
#ifdef PALMGEAR
    elems.push_back(text=new TextElement(_T("palmgear.com?67708")));
#endif

#ifdef HANDANGO
    elems.push_back(text=new TextElement(_T("handango.com/purchase, product id: 128991")));
#endif

#ifdef ARSLEXIS_VERSION
    elems.push_back(text=new TextElement(_T("our website ")));
    elems.push_back(text=new TextElement(_T("http://www.arslexis.com")));
#ifdef WIN32_PLATFORM_PSPC
    text->setHyperlink(_T("http://www.arslexis.com/pda/ppc.html"), hyperlinkExternal);
#else
    text->setHyperlink(_T("http://www.arslexis.com/pda/sm.html"), hyperlinkExternal);
#endif
#endif
    elems.push_back(new LineBreakElement());

#ifdef WIN32_PLATFORM_PSPC
    elems.push_back(text=new TextElement(_T("After obtaining registration code use menu item 'Options/Register' (or ")));
    elems.push_back(text=new TextElement(_T("click here")));
    text->setHyperlink(_T("Register"), hyperlinkTerm);
    text->setActionCallback( registerActionCallback, NULL );
    elems.push_back(text=new TextElement(_T(") to enter registration code. ")));
#else
    elems.push_back(text=new TextElement(_T("After obtaining registration code use menu item 'Menu/Register'")));    
    elems.push_back(text=new TextElement(_T(" to enter registration code. ")));
#endif

    elems.push_back(new LineBreakElement());

    elems.push_back(text=new TextElement(_T("Home.")));

    text->setJustification(DefinitionElement::justifyLeft);
    text->setHyperlink(_T("Main screen"), hyperlinkTerm);
    text->setActionCallback( aboutActionCallback, NULL );

    def->setModel(model, Definition::ownModel);
}

void prepareWikipedia(Definition *def)
{
	DefinitionModel* model = new DefinitionModel();
    Definition::Elements_t& elems = model->elements;
    TextElement* text;

    assert( def->empty() );

    FontEffects fxBold;
    fxBold.setWeight(FontEffects::weightBold);

    elems.push_back(text=new TextElement(_T("Home.")));
    text->setJustification(DefinitionElement::justifyLeft);
    text->setHyperlink(_T("Main screen"), hyperlinkTerm);
    text->setActionCallback( aboutActionCallback, NULL);
    elems.push_back(new LineBreakElement(2,3));

    elems.push_back(text=new TextElement(_T("All the articles in iPedia come from WikiPedia project and are licensed under ")));
    elems.push_back(text=new TextElement(_T("GNU Free Documentation License")));

    text->setHyperlink(_T("http://www.gnu.org/copyleft/fdl.html"), hyperlinkExternal);
    elems.push_back(text=new TextElement(_T(".")));
    elems.push_back(new LineBreakElement());

    elems.push_back(text=new TextElement(_T("To find out more about WikiPedia project, visit ")));
    elems.push_back(text=new TextElement(_T("wikipedia.org")));
    text->setHyperlink(_T("http://www.wikipedia.org"), hyperlinkExternal);
    elems.push_back(text=new TextElement(_T(" website. ")));
    elems.push_back(new LineBreakElement());

    elems.push_back(text=new TextElement(_T("Home.")));
    text->setJustification(DefinitionElement::justifyLeft);
    text->setHyperlink(_T("Main screen"), hyperlinkTerm);
    text->setActionCallback( aboutActionCallback, NULL );

    def->setModel(model, Definition::ownModel);
}    
