#include "RenderingPreferences.hpp"
#include "sm_ipedia.h"
//#include <PrefsStore.hpp>
#include <BaseTypes.hpp>

using ArsLexis::FontEffects;
using ArsLexis::Graphics;
using ArsLexis::Font;

RenderingPreferences::RenderingPreferences():
    standardIndentation_(16),
    bulletIndentation_(2),
    backgroundColor_(RGB(255,255,255))
{
    FontEffects fx;
    LOGFONT logfnt;
    HFONT fnt=(HFONT)GetStockObject(SYSTEM_FONT);
    GetObject(fnt, sizeof(logfnt), &logfnt);
    logfnt.lfHeight+=1;
    logfnt.lfCharSet = EASTEUROPE_CHARSET;
    WinFont wfnt = WinFont(CreateFontIndirect(&logfnt));
    
    fx.setUnderline(FontEffects::underlineNone);
    wfnt.setEffects(fx);   
    styles_[styleDefault].font = wfnt;
 
    
    fx.setUnderline(FontEffects::underlineDotted);
    for (uint_t i=0; i<hyperlinkTypesCount_; ++i) 
    {
        //hyperlinkDecorations_[i].font=WinFont(CreateFontIndirect(&logfnt));
        hyperlinkDecorations_[i].font.setEffects(fx);
    }
    hyperlinkDecorations_[hyperlinkTerm].textColor=RGB(0,0,250);
    hyperlinkDecorations_[hyperlinkExternal].textColor=RGB(200,20,20);     
    
    logfnt.lfWeight=800;
    logfnt.lfHeight-=1;
    styles_[styleHeader].font=WinFont(CreateFontIndirect(&logfnt));
    fx.setUnderline(FontEffects::underlineNone);
    styles_[styleHeader].font.setEffects(fx);   

    calculateIndentation();
}

void RenderingPreferences::calculateIndentation()
{
    const ArsLexis::char_t* bullet=_T("\x95");
    Graphics graphics(GetDC(hwndMain), hwndMain);
    standardIndentation_=graphics.textWidth(bullet, 1)+bulletIndentation_;
}

/*Err RenderingPreferences::serializeOut(ArsLexis::PrefsStoreWriter& writer, int uniqueId) const
{
    Err error;
    if (errNone!=(error=writer.ErrSetUInt16(uniqueId++, backgroundColor_)))
        goto OnError;
    if (errNone!=(error=writer.ErrSetUInt16(uniqueId++, bulletIndentation_)))
        goto OnError;
    for (uint_t i=0; i<hyperlinkTypesCount_; ++i)
    {
        if (errNone!=(error=hyperlinkDecorations_[i].serializeOut(writer, uniqueId)))
            goto OnError;
        uniqueId+=StyleFormatting::reservedPrefIdCount;
    }
    for (uint_t i=0; i<stylesCount_; ++i)
    {
        if (errNone!=(error=styles_[i].serializeOut(writer, uniqueId)))
            goto OnError;
        uniqueId+=StyleFormatting::reservedPrefIdCount;
    }
OnError:
    assert(errNone==error);
    return error;    
}

Err RenderingPreferences::serializeIn(ArsLexis::PrefsStoreReader& reader, int uniqueId)
{
    Err                  error;
    RenderingPreferences tmp;
    UInt16               val;

    if (errNone!=(error=reader.ErrGetUInt16(uniqueId++, &val)))
        goto OnError;
    tmp.setBackgroundColor(val);        
    if (errNone!=(error=reader.ErrGetUInt16(uniqueId++, &val)))
        goto OnError;
    tmp.setBulletIndentation(val);
    for (uint_t i=0; i<hyperlinkTypesCount_; ++i)
    {
        if (errNone!=(error=tmp.hyperlinkDecorations_[i].serializeIn(reader, uniqueId)))
            goto OnError;
        uniqueId+=StyleFormatting::reservedPrefIdCount;
    }
    for (uint_t i=0; i<stylesCount_; ++i)
    {
        if (errNone!=(error=tmp.styles_[i].serializeIn(reader, uniqueId)))
            goto OnError;
        uniqueId+=StyleFormatting::reservedPrefIdCount;
    }
    (*this)=tmp;
OnError:
    return error;    
}

Err RenderingPreferences::StyleFormatting::serializeOut(ArsLexis::PrefsStoreWriter& writer, int uniqueId) const
{
    Err error;
    if (errNone!=(error=writer.ErrSetUInt16(uniqueId++, font.fontId())))
        goto OnError;
    if (errNone!=(error=writer.ErrSetUInt16(uniqueId++, font.effects().mask())))
        goto OnError;
    if (errNone!=(error=writer.ErrSetUInt16(uniqueId++, textColor)))
        goto OnError;
OnError:
    return error;
}


Err RenderingPreferences::StyleFormatting::serializeIn(ArsLexis::PrefsStoreReader& reader, int uniqueId)
{
    Err             error;
    StyleFormatting tmp;
    UInt16          val;

    if (errNone!=(error=reader.ErrGetUInt16(uniqueId++, &val)))
        goto OnError;
    tmp.font.setFontId(static_cast<FontID>(val));
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
*/