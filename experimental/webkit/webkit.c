/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2014 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <webkit/webkit.h>
#include <debug/debug.h>
#include "src/typedefs.h"
#include "db/db.h"
#include "sample.h"
#include "model.h"
#include "table.h"
#include "util.h"
#include "utils/ayyi_utils.h"

SamplecatModel* model = NULL;

struct _backend backend; 
typedef struct _Application Application;
Application*     app = NULL;

#define HTML_DIR "html/"
#define MAX_DISPLAY_ROWS 20

static WebKitDOMDocument* document = NULL;
static WebKitDOMNode* main_node = NULL;


#if 0
static void
plugins_clicked_cb (WebKitDOMEventTarget* target, WebKitDOMEvent* event, gpointer user_data)
{
	wgp_util_remove_all_children (main_node);
}
#endif

static void
build_about (WebKitDOMNode* about_node)
{
	WebKitDOMElement* icon = webkit_dom_document_create_element (document, "img", NULL);
	//webkit_dom_element_set_attribute (icon, "src", "/usr/share/icons/Faenza/apps/32/audiobook.png", NULL);
	gchar* cwd = g_get_current_dir();
	gchar* icon_path = g_build_filename(cwd, "../../icons/samplecat.png", NULL);
	webkit_dom_element_set_attribute (icon, "src", icon_path, NULL);
	g_free(cwd);
	g_free(icon_path);
	webkit_dom_element_set_attribute (icon, "title", "About", NULL);
	webkit_dom_element_set_attribute (icon, "onClick", "$('#about_dialog').dialog('open');", NULL);
	webkit_dom_node_append_child (about_node, WEBKIT_DOM_NODE (icon), NULL);

	WebKitDOMElement* element = webkit_dom_document_create_element (document, "p", NULL);
	gchar* text = g_strdup_printf ("%s - %s", PACKAGE_STRING, "Experimental Samplecat client using WebKitGTK.");
	webkit_dom_node_set_text_content (WEBKIT_DOM_NODE (element), text, NULL);

	WebKitDOMNode* about_dialog_node = WEBKIT_DOM_NODE (webkit_dom_document_get_element_by_id (document, "about_dialog"));
	webkit_dom_node_append_child (about_dialog_node, WEBKIT_DOM_NODE (element), NULL);
}


static void
web_view_on_loaded (WebKitWebView* view, WebKitWebFrame* frame, gpointer user_data)
{
	document = webkit_web_view_get_dom_document (view);

	main_node = WEBKIT_DOM_NODE (webkit_dom_document_get_element_by_id (document, "main"));

	WebKitDOMNode* about_node = WEBKIT_DOM_NODE (webkit_dom_document_get_element_by_id (document, "about"));
	build_about (about_node);

	//webkit_dom_node_append_child (main_node, WEBKIT_DOM_NODE(webkit_dom_document_create_element(document, "input", NULL)), NULL);

	table_document = document;
	Table* t = table_new((WebKitDOMElement*)main_node, "samples");

	WebKitDOMElement* tr0 = webkit_dom_document_create_element (document, "tr", NULL);
	webkit_dom_node_append_child ((WebKitDOMNode*)t->thead, WEBKIT_DOM_NODE (tr0), NULL);
	WebKitDOMElement* th = webkit_dom_document_create_element (document, "th", NULL);
	webkit_dom_node_append_child (WEBKIT_DOM_NODE(tr0), WEBKIT_DOM_NODE (th), NULL);
	webkit_dom_node_set_text_content (WEBKIT_DOM_NODE (th), g_strdup("name"), NULL);
	th = webkit_dom_document_create_element (document, "th", NULL);
	webkit_dom_node_append_child (WEBKIT_DOM_NODE(tr0), WEBKIT_DOM_NODE (th), NULL);
	webkit_dom_node_set_text_content (WEBKIT_DOM_NODE (th), g_strdup("length"), NULL);

#ifdef USE_MYSQL
	mysql__init(model, &(SamplecatMysqlConfig){
		"localhost",
		"samplecat",
		"samplecat",
		"samplecat",
	});
	if(samplecat_set_backend(BACKEND_MYSQL)){
	}
#endif

	model->filters.search->value = g_strdup("909");

	int n_results = 0;
	if(!backend.search_iter_new("", "", &n_results)){
	}
	unsigned long* lengths;
	Sample* result;
	int row_count = 0;
	//dbg(0, "adding items to model...");
	while((result = backend.search_iter_next(&lengths)) && row_count < MAX_DISPLAY_ROWS){
		dbg(2, "  %s", result->name);

		gchar* text = g_strdup(result->name); //TODO needs freeing?

		WebKitDOMElement* tr = webkit_dom_document_create_element (document, "tr", NULL);
		webkit_dom_node_append_child (WEBKIT_DOM_NODE(t->tbody), WEBKIT_DOM_NODE (tr), NULL);
		WebKitDOMElement* td = webkit_dom_document_create_element (document, "td", NULL);
		webkit_dom_node_append_child (WEBKIT_DOM_NODE(tr), WEBKIT_DOM_NODE (td), NULL);
		webkit_dom_node_set_text_content (WEBKIT_DOM_NODE (td), text, NULL);
		td = webkit_dom_document_create_element (document, "td", NULL);
		webkit_dom_node_append_child (WEBKIT_DOM_NODE(tr), WEBKIT_DOM_NODE (td), NULL);
		webkit_dom_node_set_text_content (WEBKIT_DOM_NODE (td), g_strdup_printf("%Li", result->length), NULL);

		row_count++;
	}
	dbg(0, "n_results=%i", n_results);

	WebKitDOMElement* script = webkit_dom_document_create_element (document, "script", NULL);
	webkit_dom_node_set_text_content ( WEBKIT_DOM_NODE (script), "do_table();", NULL);
	webkit_dom_node_append_child (main_node, WEBKIT_DOM_NODE (script), NULL);

	bool on_navigation_requested(WebKitWebView* web_view, WebKitWebFrame* frame, WebKitNetworkRequest* request, WebKitWebNavigationAction* action, WebKitWebPolicyDecision* decision, gpointer user_data)
	{
		const gchar* uri = webkit_network_request_get_uri(request);
		if(!strcmp(uri, "file:///search")){
			// search button was pressed
			dbg(0, "TODO carry out new search.");
			webkit_web_policy_decision_ignore(decision);
			return true;
		}
		dbg(0, "unhandled click. request=%s", uri);
		return false; // use default behaviour
	}
	g_signal_connect(view, "navigation-policy-decision-requested", (GCallback)on_navigation_requested, NULL);
}


static void
web_view_on_console (WebKitWebView* view, char* message, int line_number, char* source_id, gpointer user_data)
{
	printf("%s%s%s\n", yellow, message, white);
}


gint
main (gint argc, gchar** argv)
{
	gtk_init (&argc, &argv);
	memset(&backend, 0, sizeof(struct _backend));

	model = samplecat_model_new();

	samplecat_model_add_filter (model, model->filters.search   = samplecat_filter_new("search"));
	samplecat_model_add_filter (model, model->filters.dir      = samplecat_filter_new("directory"));
	samplecat_model_add_filter (model, model->filters.category = samplecat_filter_new("category"));

	gchar* cwd = g_get_current_dir();
	gchar* html_path = g_build_filename(cwd, HTML_DIR "index.html", NULL);
	printf("path=%s\n", html_path);
	gchar* uri_html = g_filename_to_uri(html_path, NULL, NULL);
	g_free(cwd);

	GtkWidget* main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (main_window), "Samplecat");
	GtkWidget* scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	GtkWidget* web_view = webkit_web_view_new ();

	gtk_container_add (GTK_CONTAINER (scrolled_window), web_view);
	gtk_container_add (GTK_CONTAINER (main_window), scrolled_window);

	webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view), uri_html);
	g_signal_connect (web_view, "document-load-finished", G_CALLBACK (web_view_on_loaded), NULL);
	g_signal_connect (web_view, "console-message", G_CALLBACK (web_view_on_console), NULL);

	gtk_window_set_default_size (GTK_WINDOW (main_window), 800, 600);
	gtk_widget_show_all (main_window);

	g_signal_connect (main_window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

	gtk_main ();

	return 0;
}
