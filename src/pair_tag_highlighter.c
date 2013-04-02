#include <geanyplugin.h>
#include "Scintilla.h"      // for the SCNotification struct
#include "SciLexer.h"

#define INDICATOR_TAGMATCH 0

/* These items are set by Geany before plugin_init() is called. */
GeanyPlugin     *geany_plugin;
GeanyData       *geany_data;
GeanyFunctions  *geany_functions;

ScintillaObject *sci;

/* Is needed for clearing highlighting after moving cursor out
 * from the tag */
gint highlightedBraces[] = {0, 0, 0, 0};

PLUGIN_VERSION_CHECK(211)

PLUGIN_SET_INFO("Pair Tag Highlighter", "Finds and highlights matching \
                opening/closing HTML tag", "1.0", 
                "Volodymyr Kononenko <vm@kononenko.ws>");

/* Searches tag braces.
 * direction variable shows sets search direction:
 * TRUE  - to the right
 * FALSE - to the left
 * from the current cursor position to the start of the line.
 */
gint findBrace(gint position, gint endOfSearchPos, gchar searchedBrace, 
                gchar breakBrace, gboolean direction)
{
    gint foundBrace = -1;
    gint pos;

    if (TRUE == direction)
    {
        /* search to the right */
        for(pos=position; pos<=endOfSearchPos; pos++)
        {
            gchar charAtCurPosition = sci_get_char_at(sci, pos);
            if (charAtCurPosition == searchedBrace)
            {
                foundBrace = pos;
                break;
            }
            if (charAtCurPosition == breakBrace)
                break;
        }
    }
    else
    {
        /* search to the left */
        for(pos=position-1; pos>=endOfSearchPos; pos--)
        {
            gchar charAtCurPosition = sci_get_char_at(sci, pos);
            if (charAtCurPosition == searchedBrace)
            {
                foundBrace = pos;
                break;
            }
            if (charAtCurPosition == breakBrace)
                break;
        }
    }

    return foundBrace;
}

/* The intensity of each colour is set in the range 0 to 255.
 * If you have three such intensities, they are combined as:
 * red | (green << 8) | (blue << 16)*/
unsigned long createRGB(int r, int g, int b)
{
    return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}

void highlight_tag(gint openingBrace, gint closingBrace)
{
    scintilla_send_message(sci, SCI_INDICSETSTYLE,
                            INDICATOR_TAGMATCH, INDIC_ROUNDBOX);
    scintilla_send_message(sci, SCI_INDICSETFORE,
                            0, createRGB(0, 208, 0)); // green #00d000
    scintilla_send_message(sci, SCI_INDICSETALPHA, INDICATOR_TAGMATCH, 60);
    scintilla_send_message(sci, SCI_INDICATORFILLRANGE,
                            openingBrace, closingBrace-openingBrace+1);
}

void clear_previous_highlighting(gint rangeStart, gint rangeEnd)
{
    scintilla_send_message(sci, SCI_INDICATORCLEARRANGE, rangeStart, rangeEnd+1);
}

gboolean is_tag_self_closing(gint closingBrace)
{
    gboolean isTagSelfClosing = FALSE;
    gchar charBeforeBrace = sci_get_char_at(sci, closingBrace-1);
    if ('/' == charBeforeBrace)
        isTagSelfClosing = TRUE;
    return isTagSelfClosing;
}

gboolean is_tag_opening(openingBrace)
{
    gboolean isTagOpening = TRUE;
    gchar charAfterBrace = sci_get_char_at(sci, openingBrace+1);
    if ('/' == charAfterBrace)
        isTagOpening = FALSE;
    return isTagOpening;
}

void get_tag_name(gint openingBrace, gint closingBrace,
                    gchar tagName[], gboolean isTagOpening)
{
    gint nameStart = openingBrace + (TRUE == isTagOpening ? 1 : 2);
    gint nameEnd = nameStart;
    gchar charAtCurPosition;
    while (' ' != charAtCurPosition && '>' != charAtCurPosition)
    {
        charAtCurPosition = sci_get_char_at(sci, nameEnd);
        nameEnd++;
    }
    sci_get_text_range(sci, nameStart, nameEnd-1, tagName);
}

void findMatchingOpeningTag(gchar *tagName, gint openingBrace)
{
    gint pos;
    gint openingTagsCount = 0;
    gint closingTagsCount = 1;

    for (pos=openingBrace; pos>0; pos--)
    {
        /* are we inside tag? */
        gint lineNumber = sci_get_line_from_position(sci, pos);
        gint lineStart = sci_get_position_from_line(sci, lineNumber);
        gint matchingOpeningBrace = findBrace(pos, lineStart, '<', NULL, FALSE);
        gint matchingClosingBrace = findBrace(pos, lineStart, '>', NULL, FALSE);

        if (-1 != matchingOpeningBrace && -1 != matchingClosingBrace && (matchingClosingBrace > matchingOpeningBrace))
        {
            /* we are inside of some tag. Let us check what tag*/
            gchar matchingTagName[64];
            gboolean isMatchingTagOpening = is_tag_opening(matchingOpeningBrace);
            get_tag_name(matchingOpeningBrace, matchingClosingBrace, matchingTagName, isMatchingTagOpening);
            if (strcmp(tagName, matchingTagName) == 0)
            {
                if (TRUE == isMatchingTagOpening)
                    openingTagsCount++;
                else
                    closingTagsCount++;
            }
            pos = matchingOpeningBrace+1;
        }
        /* Speed up search: if findBrace returns -1, that means start of line
         * is reached. There is no need to go through the same positions again.
         * Jump to the start of line */
        else if (-1 == matchingOpeningBrace && -1 == matchingClosingBrace)
        {
            pos = lineStart;
            continue;
        }
        if(openingTagsCount == closingTagsCount)
        {
            /* matching tag is found */
            highlight_tag(matchingOpeningBrace, matchingClosingBrace);
            highlightedBraces[2] = matchingOpeningBrace;
            highlightedBraces[3] = matchingClosingBrace;
            break;
        }
    }
}

void findMatchingClosingTag(gchar *tagName, gint closingBrace)
{
    gint pos;
    gint linesInDocument = sci_get_line_count(sci);
    gint endOfDocument = sci_get_position_from_line(sci, linesInDocument);
    gint openingTagsCount = 1;
    gint closingTagsCount = 0;

    for (pos=closingBrace; pos<endOfDocument; pos++)
    {
        /* are we inside tag? */
        gint lineNumber = sci_get_line_from_position(sci, pos);
        gint lineEnd = sci_get_line_end_position(sci, lineNumber);
        gint matchingOpeningBrace = findBrace(pos, lineEnd, '<', NULL, TRUE);
        gint matchingClosingBrace = findBrace(pos, lineEnd, '>', NULL, TRUE);

        if (-1 != matchingOpeningBrace && -1 != matchingClosingBrace && (matchingClosingBrace > matchingOpeningBrace))
        {
            /* we are inside of some tag. Let us check what tag*/
            gchar matchingTagName[64];
            gboolean isMatchingTagOpening = is_tag_opening(matchingOpeningBrace);
            get_tag_name(matchingOpeningBrace, matchingClosingBrace, matchingTagName, isMatchingTagOpening);
            if (strcmp(tagName, matchingTagName) == 0)
            {
                if (TRUE == isMatchingTagOpening)
                    openingTagsCount++;
                else
                    closingTagsCount++;
            }
            pos = matchingClosingBrace;
        }
        /* Speed up search: if findBrace returns -1, that means end of line
         * is reached. There is no need to go through the same positions again.
         * Jump to the end of line */
        else if (-1 == matchingOpeningBrace && -1 == matchingClosingBrace)
        {
            gint lineNumber = sci_get_line_from_position(sci, pos);
            gint lineEnd = sci_get_line_end_position(sci, lineNumber);
            pos = lineEnd;
            continue;
        }
        if(openingTagsCount == closingTagsCount)
        {
            /* matching tag is found */
            highlight_tag(matchingOpeningBrace, matchingClosingBrace);
            highlightedBraces[2] = matchingOpeningBrace;
            highlightedBraces[3] = matchingClosingBrace;
            break;
        }
    }
}

void findMatchingTag(openingBrace, closingBrace)
{
    gchar tagName[64];
    gboolean isTagOpening = is_tag_opening(openingBrace);
    get_tag_name(openingBrace, closingBrace, tagName, isTagOpening);

    if(TRUE == isTagOpening)
        findMatchingClosingTag(tagName, closingBrace);
    else
        findMatchingOpeningTag(tagName, openingBrace);
}

void run_tag_highlighter()
{
    gint position = sci_get_current_position(sci);
    gint lineNumber = sci_get_current_line(sci);
    gint lineStart = sci_get_position_from_line(sci, lineNumber);
    gint lineEnd = sci_get_line_end_position(sci, lineNumber);
    gint openingBrace = findBrace(position, lineStart, '<', '>', FALSE);
    gint closingBrace = findBrace(position, lineEnd, '>', '<', TRUE);

    if (-1 == openingBrace || -1 == closingBrace)
    {
        clear_previous_highlighting(highlightedBraces[0], highlightedBraces[1]);
        clear_previous_highlighting(highlightedBraces[2], highlightedBraces[3]);
        int i;
        for (i=0; i<3; i++)
            highlightedBraces[i] = 0;
        return;
    }

    if (openingBrace != highlightedBraces[0] ||
        closingBrace != highlightedBraces[1])
    {
        clear_previous_highlighting(highlightedBraces[0], highlightedBraces[1]);
        clear_previous_highlighting(highlightedBraces[2], highlightedBraces[3]);
    }

    highlightedBraces[0] = openingBrace;
    highlightedBraces[1] = closingBrace;

    /* Highlight current tag. Matching tag will be highlighted from
     * findMatchingTag() functiong */
    highlight_tag(openingBrace, closingBrace);

    /* Find matching tag only if a tag is not self-closing */
    if (FALSE == is_tag_self_closing(closingBrace))
        findMatchingTag(openingBrace, closingBrace);
}

/* Notification handler for editor-notify */
static gboolean on_editor_notify(GObject *obj, GeanyEditor *editor,
                                SCNotification *nt, gpointer user_data)
{
    gint lexer;

    /* setting global sci variable to be available in other functions */
    sci = editor->sci;

    scintilla_send_message(sci, SCI_SETINDICATORCURRENT, INDICATOR_TAGMATCH, 0);

    lexer = sci_get_lexer(sci);
    if (lexer != SCLEX_HTML && lexer != SCLEX_XML)
    {
        return FALSE;
    }

    /* nmhdr is a structure containing information about the event */
    switch (nt->nmhdr.code)
    {
        case SCN_UPDATEUI:
            run_tag_highlighter();
            break;
    }

    /* returning FALSE to allow Geany processing the event */
    return FALSE;
}

PluginCallback plugin_callbacks[] =
{
    { "editor-notify", (GCallback) &on_editor_notify, FALSE, NULL },
    { NULL, NULL, FALSE, NULL }
};

void plugin_init(GeanyData *data)
{
}

void plugin_cleanup(void)
{
}
