#include <geanyplugin.h>
#include "Scintilla.h"      // for the SCNotification struct
#include "SciLexer.h"

/* These items are set by Geany before plugin_init() is called. */
GeanyPlugin     *geany_plugin;
GeanyData       *geany_data;
GeanyFunctions  *geany_functions;

PLUGIN_VERSION_CHECK(211)

PLUGIN_SET_INFO("Pair Tag Highlighter", "Finds and highlights matching \
                opening/closing HTML tag", "1.0", 
                "Volodymyr Kononenko <vm@kononenko.ws>");

void run_tag_highlighter(ScintillaObject *sci)
{
    gint position = sci_get_current_position(sci);

    /* test styling */
    scintilla_send_message(sci, SCI_STARTSTYLING, position, 0x1f);
    scintilla_send_message(sci, SCI_SETSTYLING, 3, 36);
    scintilla_send_message(sci, SCI_STYLESETBOLD, 3, 1);
}

/* Notification handler for editor-notify */
static gboolean on_editor_notify(GObject *obj, GeanyEditor *editor,
                                SCNotification *nt, gpointer user_data)
{
    gint lexer;

    lexer = sci_get_lexer(editor->sci);
    if (lexer != SCLEX_HTML && lexer != SCLEX_XML)
    {
        return FALSE;
    }

    /* nmhdr is a structure containing information about the event */
    switch (nt->nmhdr.code)
    {
        case SCN_UPDATEUI:
            run_tag_highlighter(editor->sci);
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
