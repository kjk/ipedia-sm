#include "RenderingPreferences.hpp"
#include "sm_ipedia.h"
#include "iPediaApplication.hpp"
#include <WinPrefsStore.hpp>
#include <BaseTypes.hpp>

using ArsLexis::FontEffects;
using ArsLexis::Graphics;
using ArsLexis::Font;
using ArsLexis::status_t;

RenderingPreferences::RenderingPreferences():
    standardIndentation_(16),
    bulletIndentation_(2),
#ifdef DEBUG
    backgroundColor_(RGB(12,255,12))
#else
    backgroundColor_(RGB(255,255,255))
#endif
{
    FontEffects fx;

    int fontSize = 12;

    WinFont wfnt = WinFont(fontSize);

    //fx.setUnderline(FontEffects::underlineNone);
    //wfnt.setEffects(fx);   
    styles_[styleDefault].font = wfnt;
     
    fx.setUnderline(FontEffects::underlineDotted);
    for (uint_t i=0; i<hyperlinkTypesCount_; ++i) 
        hyperlinkDecorations_[i].font.setEffects(fx);

    hyperlinkDecorations_[hyperlinkTerm].textColor=RGB(0,0,250);
    hyperlinkDecorations_[hyperlinkExternal].textColor=RGB(200,20,20);     
    hyperlinkDecorations_[hyperlinkBookmark].textColor=RGB(0,0,180);     
#ifdef WIN32_PLATFORM_PSPC
    styles_[styleHeader].font=WinFont(fontSize+3);
#else
    styles_[styleHeader].font=WinFont(fontSize+2);
#endif
    fx.setUnderline(FontEffects::underlineNone);
    fx.setWeight(FontEffects::weightBlack);
    styles_[styleHeader].font.setEffects(fx);   

    calculateIndentation();
}

void RenderingPreferences::calculateIndentation()
{
    const ArsLexis::char_t* bullet=_T("\x95");
    iPediaApplication &app = iPediaApplication::instance();
    Graphics graphics(GetDC(app.getMainWindow()), app.getMainWindow());
    standardIndentation_=graphics.textWidth(bullet, 1)+bulletIndentation_;
}

status_t RenderingPreferences::serializeOut(ArsLexis::PrefsStoreWriter& writer, int uniqueId) const
{
    /*status_t error;
    if (errNone!=(error=writer.ErrSetUInt32(uniqueId++, backgroundColor_)))
        goto OnError;
    if (errNone!=(error=writer.ErrSetUInt16(uniqueId++, bulletIndentation_)))
        goto OnError;
    uint_t i;
    for (i=0; i<hyperlinkTypesCount_; ++i)
    {
        if (errNone!=(error=hyperlinkDecorations_[i].serializeOut(writer, uniqueId)))
            goto OnError;
        uniqueId+=StyleFormatting::reservedPrefIdCount;
    }
    for (i=0; i<stylesCount_; ++i)
    {
        if (errNone!=(error=styles_[i].serializeOut(writer, uniqueId)))
            goto OnError;
        uniqueId+=StyleFormatting::reservedPrefIdCount;
    }
OnError:
    assert(errNone==error);
    return error;    */
    return errNone;
}

status_t RenderingPreferences::serializeIn(ArsLexis::PrefsStoreReader& reader, int uniqueId)
{
/*    status_t                  error;
    RenderingPreferences tmp;
    ushort_t               val;

    if (errNone!=(error=reader.ErrGetUInt16(uniqueId++, &val)))
        goto OnError;
    tmp.setBackgroundColor(val);        
    if (errNone!=(error=reader.ErrGetUInt16(uniqueId++, &val)))
        goto OnError;
    tmp.setBulletIndentation(val);
    uint_t i;
    for (i=0; i<hyperlinkTypesCount_; ++i)
    {
        if (errNone!=(error=tmp.hyperlinkDecorations_[i].serializeIn(reader, uniqueId)))
            goto OnError;
        uniqueId+=StyleFormatting::reservedPrefIdCount;
    }
    for (i=0; i<stylesCount_; ++i)
    {
        if (errNone!=(error=tmp.styles_[i].serializeIn(reader, uniqueId)))
            goto OnError;
        uniqueId+=StyleFormatting::reservedPrefIdCount;
    }
    (*this)=tmp;
OnError:
    return error;    */
    return errNone;
}

status_t RenderingPreferences::StyleFormatting::serializeOut(ArsLexis::PrefsStoreWriter& writer, int uniqueId) const
{
    status_t error;
    //if (errNone!=(error=writer.ErrSetUInt16(uniqueId++, font.fontId())))
        //goto OnError;
    if (errNone!=(error=writer.ErrSetUInt16(uniqueId++, font.effects().mask())))
        goto OnError;
    if (errNone!=(error=writer.ErrSetUInt32(uniqueId++, textColor)))
        goto OnError;
OnError:
    return error;
}


status_t RenderingPreferences::StyleFormatting::serializeIn(ArsLexis::PrefsStoreReader& reader, int uniqueId)
{
    status_t             error;
    StyleFormatting tmp;
    ushort_t          val;

    //if (errNone!=(error=reader.ErrGetUInt16(uniqueId++, &val)))
        //goto OnError;
    //tmp.font.setFontId(static_cast<FontID>(val));
    if (errNone!=(error=reader.ErrGetUInt16(uniqueId++, &val)))
        goto OnError;
    tmp.font.setEffects(ArsLexis::FontEffects(val));
    if (errNone!=(error=reader.ErrGetUInt16(uniqueId++, &val)))
        goto OnError;
    tmp.textColor=val;
    (*this)=tmp;
OnError:    
    return error;
}
