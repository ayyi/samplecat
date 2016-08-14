/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2015 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <debug/debug.h>
#include "samplecat.h"
#include <webkit/webkit.h> // included _after_ samplecat due to redefine of bool
#include "table.h"
#include "util.h"
#include "src/support.h"

#define MODEL samplecat.model
#define backend samplecat.model->backend

typedef struct _Application {
   ConfigContext   config_ctx;
   Config          config;
} Application;
Application app = {{0,},};
GList* samples = NULL;

#define HTML_DIR "html/"
#define MAX_DISPLAY_ROWS 20

static GtkWidget* web_view = NULL;

struct {
	WebKitDOMDocument* document;
	WebKitDOMNode*     main;
	WebKitDOMElement*  results;
	Table*             table;
} dom = {0,};


static void    add_web_inspector (GtkWidget* web_view, GtkWidget* paned);
static void    build_about       (WebKitDOMNode* about_node);
static Sample* find_sample_by_id (int);


#if 0
static void
plugins_clicked_cb (WebKitDOMEventTarget* target, WebKitDOMEvent* event, gpointer user_data)
{
	wgp_util_remove_all_children (main_node);
}
#endif

static void
show_results()
{
	Table* t = dom.table;

	unsigned long* lengths;
	Sample* result;
	int row_count = 0;
	while((result = backend.search_iter_next(&lengths)) && row_count < MAX_DISPLAY_ROWS){
		dbg(2, "  %s", result->name);

		Sample* s = sample_dup(result);
		samples = g_list_append(samples, s); // call sample_unref() when removing from list.

		gchar* text = g_strdup(result->name);
		char length[64]; format_smpte(length, result->length);

		WebKitDOMElement* tr = webkit_dom_document_create_element (dom.document, "tr", NULL);
		webkit_dom_node_append_child (WEBKIT_DOM_NODE(t->tbody), WEBKIT_DOM_NODE (tr), NULL);

		WebKitDOMElement* td = webkit_dom_document_create_element (dom.document, "td", NULL);
		webkit_dom_node_append_child (WEBKIT_DOM_NODE(tr), WEBKIT_DOM_NODE (td), NULL);
		webkit_dom_node_set_text_content (WEBKIT_DOM_NODE (td), text, NULL);

		td = webkit_dom_document_create_element (dom.document, "td", NULL);
		webkit_dom_node_append_child (WEBKIT_DOM_NODE(tr), WEBKIT_DOM_NODE (td), NULL);
		webkit_dom_node_set_text_content (WEBKIT_DOM_NODE (td), dir_format(result->full_path), NULL);

		td = webkit_dom_document_create_element (dom.document, "td", NULL);
		webkit_dom_node_append_child (WEBKIT_DOM_NODE(tr), WEBKIT_DOM_NODE (td), NULL);
		WebKitDOMElement* img = webkit_dom_document_create_element (dom.document, "img", NULL);
		char thumbnail[32] = {0,};
		snprintf(thumbnail, 31, "file:///thumbnail-%i.png", result->id);
		webkit_dom_element_set_attribute (img, "src", thumbnail, NULL);
		webkit_dom_element_set_attribute (img, "width", "200", NULL);
		webkit_dom_element_set_attribute (img, "height", "20", NULL);
		webkit_dom_node_append_child (WEBKIT_DOM_NODE(td), WEBKIT_DOM_NODE(img), NULL);

		td = webkit_dom_document_create_element (dom.document, "td", NULL);
		webkit_dom_node_append_child (WEBKIT_DOM_NODE(tr), WEBKIT_DOM_NODE (td), NULL);
		webkit_dom_node_set_text_content (WEBKIT_DOM_NODE (td), length, NULL);

		td = webkit_dom_document_create_element (dom.document, "td", NULL);
		webkit_dom_node_append_child (WEBKIT_DOM_NODE(tr), WEBKIT_DOM_NODE (td), NULL);
		gchar* channels = g_strdup_printf("%u", result->channels);
		webkit_dom_node_set_text_content (WEBKIT_DOM_NODE (td), channels, NULL);
		g_free(channels);

		g_free(text);

		row_count++;
	}

	webkit_web_view_execute_script((WebKitWebView*)web_view, "do_table();");
}


static void
web_view_on_loaded (WebKitWebView* view, WebKitWebFrame* frame, gpointer user_data)
{
	dom.document = webkit_web_view_get_dom_document (view);

	dom.main = WEBKIT_DOM_NODE (webkit_dom_document_get_element_by_id (dom.document, "main"));
	g_return_if_fail(dom.main);

	WebKitDOMNode* about_node = WEBKIT_DOM_NODE (webkit_dom_document_get_element_by_id (dom.document, "about"));
	build_about (about_node);

	//webkit_dom_node_append_child (dom.main, WEBKIT_DOM_NODE(webkit_dom_document_create_element(dom.document, "input", NULL)), NULL);

	table_document = dom.document;
	Table* t = dom.table = table_new((WebKitDOMElement*)dom.main, "samples");
	//webkit_web_view_execute_script((WebKitWebView*)web_view, "do_table();");

	WebKitDOMElement* tr0 = webkit_dom_document_create_element (dom.document, "tr", NULL);
	webkit_dom_node_append_child ((WebKitDOMNode*)t->thead, WEBKIT_DOM_NODE (tr0), NULL);

	char* headings[] = {"name", "path", "", "length", "channels"};

	int i; for(i=0;i<G_N_ELEMENTS(headings);i++) {
		WebKitDOMElement* th = webkit_dom_document_create_element (dom.document, "th", NULL);
		webkit_dom_node_append_child (WEBKIT_DOM_NODE(tr0), WEBKIT_DOM_NODE (th), NULL);
		webkit_dom_node_set_text_content (WEBKIT_DOM_NODE (th), g_strdup(headings[i]), NULL);
	}

	samplecat_init();

	app.config_ctx.filename = g_strdup_printf("%s/.config/" PACKAGE "/" PACKAGE, g_get_home_dir());
	config_load(&app.config_ctx, &app.config);

	db_init(
#ifdef USE_MYSQL
		&app.config.mysql
#else
		NULL
#endif
	);

	if (app.config.database_backend && can_use(MODEL->backends, app.config.database_backend)) {
		list_clear(MODEL->backends);
		samplecat_model_add_backend(app.config.database_backend);
	}

	if(!db_connect()){
		g_warning("cannot connect to any database.\n");
		exit(1);
	}

	void on_search_filter_changed(GObject* _filter, gpointer _)
	{
		dbg(0, "search=%s", MODEL->filters.search->value);

		int n_results = 0;
		backend.search_iter_new(&n_results);
		dbg(0, "n_results=%i", n_results);
		show_results();
		backend.search_iter_free();

		WebKitDOMHTMLInputElement* search = WEBKIT_DOM_HTML_INPUT_ELEMENT (webkit_dom_document_get_element_by_id (dom.document, "search-box-input"));
		gchar* text = webkit_dom_html_input_element_get_value(search);
		if(strcmp(text, ((SamplecatFilter*)samplecat.model->filters.search)->value)){
			webkit_dom_html_input_element_set_value (search, ((SamplecatFilter*)samplecat.model->filters.search)->value);
		}
		g_free(text);
	}

	on_search_filter_changed((GObject*)samplecat.model->filters.search, NULL);

	/*
	WebKitDOMElement* script = webkit_dom_document_create_element (dom.document, "script", NULL);
	webkit_dom_node_set_text_content (WEBKIT_DOM_NODE (script), "do_table();", NULL);
	webkit_dom_node_append_child (dom.main, WEBKIT_DOM_NODE (script), NULL);
	*/

	bool on_navigation_requested(WebKitWebView* web_view, WebKitWebFrame* frame, WebKitNetworkRequest* request, WebKitWebNavigationAction* action, WebKitWebPolicyDecision* decision, gpointer user_data)
	{
		const gchar* uri = webkit_network_request_get_uri(request);
		if(!strcmp(uri, "file:///search")){
			// search button was pressed

			webkit_web_view_execute_script((WebKitWebView*)web_view, "dt.destroy(false);");
			table_clear(dom.table);

			WebKitDOMHTMLInputElement* search = WEBKIT_DOM_HTML_INPUT_ELEMENT (webkit_dom_document_get_element_by_id (dom.document, "search-box-input"));
			gchar* text = webkit_dom_html_input_element_get_value(search);
			samplecat_filter_set_value(samplecat.model->filters.search, text);
			g_free(text);

			webkit_web_policy_decision_ignore(decision);
			return true;
		}
		dbg(0, "unhandled click. request=%s", uri);
		return false; // use default behaviour
	}
	g_signal_connect(view, "navigation-policy-decision-requested", (GCallback)on_navigation_requested, NULL);

	g_signal_connect(samplecat.model->filters.search, "changed", G_CALLBACK(on_search_filter_changed), NULL);
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

	gchar* cwd = g_get_current_dir();
	gchar* html_path = g_build_filename(cwd, HTML_DIR "index.html", NULL);
	printf("path=%s\n", html_path);
	if(!g_file_test(html_path, G_FILE_TEST_EXISTS)){
		printf("***cannot load html resource\n   try running from the samplecat webkit directory\n");
		return EXIT_FAILURE;
	}
	gchar* uri_html = g_filename_to_uri(html_path, NULL, NULL);
	g_free(cwd);

	GtkWidget* main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (main_window), "Samplecat");
	GtkWidget* scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	GtkWidget* paned = gtk_vpaned_new();
	web_view = webkit_web_view_new ();

	gtk_scrolled_window_add_with_viewport ((GtkScrolledWindow*)scrolled_window, paned);
	gtk_paned_add1((GtkPaned*)paned, web_view);
	gtk_container_add (GTK_CONTAINER (main_window), scrolled_window);

	add_web_inspector(web_view, paned);

	void on_request_start (WebKitWebView* web_view, WebKitWebFrame* web_frame, WebKitWebResource* web_resource, WebKitNetworkRequest* request, WebKitNetworkResponse* response, gpointer user_data)
	{
		const char* uri = webkit_network_request_get_uri(request);
		dbg(1, "request=%s", uri);
		if(g_str_has_prefix(uri, "file:///thumbnail-")){
			gchar* copy = g_strdup(uri + 18);
			gchar* dot = g_strrstr (copy, ".");
			if(dot){
				dot[0] = '\0';
				dbg(1, "id=%i", atoi(copy));
				Sample* s = find_sample_by_id (atoi(copy));
				if(s){
					gsize buffer_size;
					gchar* buffer;
					if(gdk_pixbuf_save_to_buffer(s->overview, &buffer, &buffer_size, "png", NULL, NULL)){
						gchar* img = g_base64_encode((guchar*)buffer, buffer_size);
						gchar* data = g_strdup_printf("data:image/png;base64,%s", img);
						webkit_network_request_set_uri(request, data);
						g_free(img);
						g_free(data);
						g_free(buffer);
					}
				}
				g_free(copy);
			}
		}
	}

	g_signal_connect(web_view, "resource-request-starting", (GCallback)on_request_start, NULL);

	webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view), uri_html);
	g_signal_connect (web_view, "document-load-finished", G_CALLBACK (web_view_on_loaded), NULL);
	g_signal_connect (web_view, "console-message", G_CALLBACK (web_view_on_console), NULL);

	gtk_window_set_default_size (GTK_WINDOW (main_window), 800, 600);
	gtk_widget_show_all (main_window);

	g_signal_connect (main_window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

	gtk_main ();

	return 0;
}


static void
add_web_inspector(GtkWidget* web_view, GtkWidget* paned)
{
	// the inspector is known to be broken in many cases on webkit-gtk 2.4.9 but works on earlier versions

	WebKitWebSettings* settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW(web_view));
	g_object_set (G_OBJECT(settings), "enable-developer-extras", TRUE, NULL);

	WebKitWebView* activate_inspector (WebKitWebInspector* web_inspector, WebKitWebView* target_view, gpointer paned)
	{
		GtkWidget* web_view = webkit_web_view_new();
		gtk_paned_add2((GtkPaned*)paned, web_view);
		gtk_paned_set_position ((GtkPaned*)paned, gtk_widget_get_toplevel((GtkWidget*)target_view)->allocation.height - 100);
		return (WebKitWebView*)web_view;
	}
	/*
	gboolean show_inpector_window (WebKitWebInspector* web_inspector, gpointer user_data)
	{
		PF0;
		//Emitted when the inspector window should be displayed. Notice that the window must have been created already by handling inspect-web-view.
		return false; // NOT_HANDLED
	}
	void inspected_uri_changed(WebKitWebInspector* web_inspector)
	{
	}
	*/

	WebKitWebInspector* inspector = webkit_web_view_get_inspector (WEBKIT_WEB_VIEW(web_view));
	g_signal_connect (G_OBJECT(inspector), "inspect-web-view", G_CALLBACK(activate_inspector), paned);
	//g_signal_connect (G_OBJECT(inspector), "show-window", G_CALLBACK(show_inpector_window), NULL);
	//g_signal_connect (G_OBJECT(inspector), "notify::inspected-uri", G_CALLBACK(inspected_uri_changed), NULL);
}


static void
build_about (WebKitDOMNode* about_node)
{
	WebKitDOMElement* icon = webkit_dom_document_create_element (dom.document, "img", NULL);
	//webkit_dom_element_set_attribute (icon, "src", "/usr/share/icons/Faenza/apps/32/audiobook.png", NULL);
	gchar* cwd = g_get_current_dir();
	gchar* icon_path = g_build_filename(cwd, "../../icons/samplecat.png", NULL);
	webkit_dom_element_set_attribute (icon, "src", icon_path, NULL);
	g_free(cwd);
	g_free(icon_path);
	webkit_dom_element_set_attribute (icon, "title", "About", NULL);
	webkit_dom_element_set_attribute (icon, "onClick", "$('#about_dialog').dialog('open');", NULL);
	webkit_dom_node_append_child (about_node, WEBKIT_DOM_NODE (icon), NULL);

	WebKitDOMElement* element = webkit_dom_document_create_element (dom.document, "p", NULL);
	gchar* text = g_strdup_printf ("%s - %s", PACKAGE_STRING, "Experimental Samplecat client using WebKitGTK.");
	webkit_dom_node_set_text_content (WEBKIT_DOM_NODE (element), text, NULL);

	WebKitDOMNode* about_dialog_node = WEBKIT_DOM_NODE (webkit_dom_document_get_element_by_id (dom.document, "about_dialog"));
	webkit_dom_node_append_child (about_dialog_node, WEBKIT_DOM_NODE (element), NULL);
}


static Sample*
find_sample_by_id(int id)
{
	GList* l = samples;
	for(;l;l=l->next){
		if(((Sample*)l->data)->id == id) return (Sample*)l->data;
	}
	return NULL;
}


// temporary
void
application_emit_icon_theme_changed (Application* app, const gchar* _)
{
}


