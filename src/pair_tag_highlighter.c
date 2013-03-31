#include <geanyplugin.h>
#include "Scintilla.h"      // for the SCNotification struct
#include "SciLexer.h"

/* These items are set by Geany before plugin_init() is called. */
GeanyPlugin     *geany_plugin;
GeanyData       *geany_data;
GeanyFunctions  *geany_functions;

ScintillaObject *sci;

PLUGIN_VERSION_CHECK(211)

PLUGIN_SET_INFO("Pair Tag Highlighter", "Finds and highlights matching \
                opening/closing HTML tag", "1.0", 
                "Volodymyr Kononenko <vm@kononenko.ws>");

/* searches openging tag brace to the left: from the current cursor
 * position to the start of the line. */
gint findOpeningBrace(gint position, gint lineStart)
{
    gint openingBrace = -1;
    gint pos;
    for(pos=position; pos>=lineStart; pos--)
    {
        gchar charAtCurPosition = sci_get_char_at(sci, pos-1);
        if ('<' == charAtCurPosition)
        {
            openingBrace = pos;
            break;
        }
    }
    /* if opening brace is not found in the line, we're not inside tag */
    if (-1 != openingBrace)
        openingBrace = pos;

    return openingBrace;
}

/* searches closing tag brace to the right: from the current cursor
 * position to the end of the line. */
gint findClosingBrace(gint position, gint lineEnd)
{
    gint closingBrace = -1;
    gint pos;
    for(pos=position; pos<=lineEnd; pos++)
    {
        gchar charAtCurPosition = sci_get_char_at(sci, pos);
        if ('>' == charAtCurPosition)
        {
            closingBrace = pos;
            break;
        }
    }
    /* if closing brace is not found in the line, we're not inside tag */
    if (-1 != closingBrace)
        closingBrace = pos;

    return closingBrace;
}

void highlight_tag(gint openingBrace, gint closingBrace)
{
    scintilla_send_message(sci, SCI_STARTSTYLING, openingBrace-1, 0x1f);
    scintilla_send_message(sci, SCI_SETSTYLING, closingBrace-openingBrace+2, 36);
    scintilla_send_message(sci, SCI_STYLESETBOLD, closingBrace-openingBrace+2, 1);
}

void run_tag_highlighter()
{
    gint position = sci_get_current_position(sci);
    gint lineNumber = sci_get_current_line(sci);
    gint lineStart = sci_get_position_from_line(sci, lineNumber);
    gint lineEnd = sci_get_line_end_position(sci, lineNumber);
    gint openingBrace = findOpeningBrace(position, lineStart);
    gint closingBrace = findClosingBrace(position, lineEnd);

    if (-1 == openingBrace || -1 == closingBrace)
        return;

    /* Highlight current tag. Matching tag will be highlighted from
     * findMatchingTag() functiong */
    highlight_tag(openingBrace, closingBrace);
}

/* Notification handler for editor-notify */
static gboolean on_editor_notify(GObject *obj, GeanyEditor *editor,
                                SCNotification *nt, gpointer user_data)
{
    gint lexer;

    /* setting global sci variable to be available in other functions */
    sci = editor->sci;
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
