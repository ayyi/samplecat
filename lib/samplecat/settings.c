/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* | copyright (C) 2007-2018 Tim Orford <tim@orford.org>                  |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <string.h>
#include <gtk/gtk.h>
#include "typedefs.h"
#include "utils/ayyi_utils.h"
#include "debug/debug.h"
#include "file_manager/file_manager.h"
#include "samplecat.h"
#include "settings.h"

extern char theme_name[64];

#define PALETTE_SIZE 17 // FIXME temporary - also in src/types.h


static void
config_new(ConfigContext* ctx)
{
	//g_key_file_has_group(GKeyFile *key_file, const gchar *group_name);

	GError* error = NULL;
	char data[256 * 256];
	sprintf(data, "# this is the default config file for the Samplecat application.\n# pls enter your database details.\n"
		"[Samplecat]\n"
		"database_backend=sqlite\n"
		"mysql_host=localhost\n"
		"mysql_user=username\n"
		"mysql_pass=pass\n"
		"mysql_name=samplelib\n"
		"show_dir=\n"
		"auditioner=\n"
		"jack_autoconnect=system:playback_\n"
		"jack_midiconnect=DISABLED\n"
		);

	if(!g_key_file_load_from_data(ctx->key_file, data, strlen(data), G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error)){
		perr("error creating new key_file from data. %s\n", error->message);
		g_error_free(error);
		error = NULL;
		return;
	}

	printf("A default config file has been created. Please enter your database details in '%s'.\n", ctx->filename);
}


bool
config_load(ConfigContext* ctx, Config* config)
{
	// TODO use the ConfigOption's for loading as well as saving

#ifdef USE_MYSQL
	strcpy(config->mysql.name, "samplecat");
#endif
	strcpy(config->show_dir, "");

	int i;
	for (i=0;i<PALETTE_SIZE;i++) {
		//currently these are overridden anyway
		snprintf(config->colour[i], 7, "%s", "000000");
	}

	GError* error = NULL;
	ctx->key_file = g_key_file_new();
	if(g_key_file_load_from_file(ctx->key_file, ctx->filename, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error)){
		p_(1, "config loaded.");
		gchar* groupname = g_key_file_get_start_group(ctx->key_file);
		dbg (2, "group=%s.", groupname);
		if(!strcmp(groupname, "Samplecat")){
			int c; for(c=0;c<CONFIG_MAX;c++){
				if(c == CONFIG_ICON_THEME){
					ConfigOption* o = ctx->options[c];
					g_value_set_string(&ctx->options[CONFIG_ICON_THEME]->val, g_key_file_get_string(ctx->key_file, groupname, o->name, &error));
				}
			}
#ifdef USE_MYSQL
#define num_keys (15)
#else
#define num_keys (11)
#endif
#define ADD_CONFIG_KEY(VAR, NAME) \
			strcpy(keys[i], NAME); \
			loc[i] = VAR; \
			siz[i] = G_N_ELEMENTS(VAR); \
			i++;

			char  keys[num_keys+(PALETTE_SIZE-1)][64];
			char*  loc[num_keys+(PALETTE_SIZE-1)];
			size_t siz[num_keys+(PALETTE_SIZE-1)];

			i=0;
			ADD_CONFIG_KEY (config->database_backend, "database_backend");
#ifdef USE_MYSQL
			ADD_CONFIG_KEY (config->mysql.host,       "mysql_host");
			ADD_CONFIG_KEY (config->mysql.user,       "mysql_user");
			ADD_CONFIG_KEY (config->mysql.pass,       "mysql_pass");
			ADD_CONFIG_KEY (config->mysql.name,       "mysql_name");
#endif
			ADD_CONFIG_KEY (config->window_height,    "window_height");
			ADD_CONFIG_KEY (config->window_width,     "window_width");
			ADD_CONFIG_KEY (config->column_widths[0], "col1_width");
			ADD_CONFIG_KEY (config->browse_dir,       "browse_dir");
			ADD_CONFIG_KEY (config->show_player,      "show_player");
			ADD_CONFIG_KEY (config->show_waveform,    "show_waveform");
			ADD_CONFIG_KEY (config->show_spectrogram, "show_spectrogram");
			ADD_CONFIG_KEY (config->auditioner,       "auditioner");
			ADD_CONFIG_KEY (config->jack_autoconnect, "jack_autoconnect");
			ADD_CONFIG_KEY (config->jack_midiconnect, "jack_midiconnect");

			int k;
			for (k=0;k<PALETTE_SIZE-1;k++) {
				char tmp[16]; snprintf(tmp, 16, "colorkey%02d", k+1);
				ADD_CONFIG_KEY(config->colour[k+1], tmp)
			}

			gchar* keyval;
			for(k=0;k<(num_keys+PALETTE_SIZE-1);k++){
				if((keyval = g_key_file_get_string(ctx->key_file, groupname, keys[k], &error))){
					if(loc[k]){
						size_t keylen = siz[k];
						snprintf(loc[k], keylen, "%s", keyval); loc[k][keylen-1] = '\0';
					}
					dbg(2, "%s=%s", keys[k], keyval);
					g_free(keyval);
				}else{
					if(error->code == 3) g_error_clear(error)
					else { GERR_WARN; }
					if (!loc[k] || strlen(loc[k])==0) strcpy(loc[k], "");
				}
			}

			if((keyval = g_key_file_get_string(ctx->key_file, groupname, "show_dir", &error))){
				samplecat_model_set_search_dir (samplecat.model, config->show_dir);
			}
			if((keyval = g_key_file_get_string(ctx->key_file, groupname, "filter", &error))){
				samplecat.model->filters.search->value = g_strdup(keyval);
			}

			{
				bool keyval = g_key_file_get_boolean(ctx->key_file, groupname, "add_recursive", &error);
				if(error){
					if(error->code == 3) g_error_clear(error)
					else { GERR_WARN; }
				}else{
					config->add_recursive = keyval;
				}

				keyval = g_key_file_get_boolean(ctx->key_file, groupname, "loop_playback", &error);
				if(error){
					if(error->code == 3) g_error_clear(error)
					else { GERR_WARN; }
				}else{
					config->loop_playback = keyval;
				}
			}

#if 0
#ifndef USE_GDL
			app->view_options[SHOW_PLAYER]      = (ViewOption){"Player",      show_player,      strcmp(config->show_player, "false")};
			app->view_options[SHOW_FILEMANAGER] = (ViewOption){"Filemanager", show_filemanager, true};
			app->view_options[SHOW_WAVEFORM]    = (ViewOption){"Waveform",    show_waveform,    !strcmp(config->show_waveform, "true")};
#ifdef HAVE_FFTW3
			app->view_options[SHOW_SPECTROGRAM] = (ViewOption){"Spectrogram", show_spectrogram, !strcmp(config->show_spectrogram, "true")};
#endif
#endif
#endif
		}
		else{ pwarn("cannot find Samplecat key group.\n"); return false; }
		g_free(groupname);
	}else{
		printf("unable to load config file: %s.\n", error->message);
		g_error_free(error);
		error = NULL;
		config_new(ctx);
		config_save(ctx);
		return false;
	}

	return true;
}


bool
config_save(ConfigContext* ctx)
{
	// filter settings:
	g_key_file_set_value(ctx->key_file, "Samplecat", "show_dir", samplecat.model->filters.dir->value ? samplecat.model->filters.dir->value : "");
	g_key_file_set_value(ctx->key_file, "Samplecat", "filter", samplecat.model->filters.search->value ? samplecat.model->filters.search->value : "");

	// application specific settings:
	if(ctx->options){
		char value[256];
		int i = 0;
		ConfigOption* option;
		while((option = ctx->options[i++])){
			option->save(option);

			if(G_VALUE_HOLDS_INT(&option->val)){
				int val = g_value_get_int(&option->val);
				dbg(2, "option: %s=%i", option->name, val);
				if(val > g_value_get_int(&option->min) && val < g_value_get_int(&option->max)){
					snprintf(value, 255, "%i", val);
					g_key_file_set_value(ctx->key_file, "Samplecat", option->name, value);
				}
			}else if(G_VALUE_HOLDS_STRING(&option->val)){
				dbg(2, "option: %s=%s", option->name, g_value_get_string(&option->val));
				if(g_value_get_string(&option->val)) g_key_file_set_value(ctx->key_file, "Samplecat", option->name, g_value_get_string(&option->val));
			}else if(G_VALUE_HOLDS_BOOLEAN(&option->val)){
				dbg(2, "option: %s=%i", option->name, g_value_get_boolean(&option->val));
				//snprintf(value, 255, "%s", g_value_get_boolean(&option->val) ? "true" : "false");
				g_key_file_set_boolean(ctx->key_file, "Samplecat", option->name, g_value_get_boolean(&option->val));
			}else{
				dbg(2, "option: [manual]");
			}
		}
	}

	AyyiFilemanager* fm = file_manager__get();
	if(fm && fm->real_path){
		g_key_file_set_value(ctx->key_file, "Samplecat", "browse_dir", fm->real_path);
	}

#if 0
#ifndef USE_GDL
	g_key_file_set_value(ctx->key_file, "Samplecat", "show_player", app->view_options[SHOW_PLAYER].value ? "true" : "false");
	g_key_file_set_value(ctx->key_file, "Samplecat", "show_waveform", app->view_options[SHOW_WAVEFORM].value ? "true" : "false");
#ifdef HAVE_FFTW3
	g_key_file_set_value(ctx->key_file, "Samplecat", "show_spectrogram", app->view_options[SHOW_SPECTROGRAM].value ? "true" : "false");
#endif
#endif
#endif

	GError* error = NULL;
	gsize length;
	gchar* string = g_key_file_to_data(ctx->key_file, &length, &error);
	if(error){
		dbg (0, "error saving config file: %s", error->message);
		g_error_free(error);
	}

	if(ensure_config_dir()){

		FILE* fp;
		if(!(fp = fopen(ctx->filename, "w"))){
			logger_log(samplecat.logger, "cannot open config file for writing (%s).", ctx->filename);
			return false;
		}
		if(fprintf(fp, "%s", string) < 0){
			logger_log(samplecat.logger, "error writing data to config file (%s).", ctx->filename);
		}
		fclose(fp);
	}
	else errprintf("cannot create config directory.");
	g_free(string);

	return true;
}


ConfigOption*
config_option_new_int(char* name, void (*save)(ConfigOption*), int min, int max)
{
	ConfigOption* o = g_new0(ConfigOption, 1);
	*o = (ConfigOption){
		.name = name,
		.save = save,
	};

	g_value_init(&o->val, G_TYPE_INT);
	g_value_init(&o->min, G_TYPE_INT);
	g_value_init(&o->max, G_TYPE_INT);
	g_value_set_int(&o->min, min);
	g_value_set_int(&o->max, max);

	return o;
}


ConfigOption*
config_option_new_string(char* name, void (*save)(ConfigOption*))
{
	ConfigOption* o = g_new0(ConfigOption, 1);
	*o = (ConfigOption){
		.name = name,
		.save = save,
	};

	g_value_init(&o->val, G_TYPE_STRING);

	return o;
}


ConfigOption*
config_option_new_bool(char* name, void (*save)(ConfigOption*))
{
	ConfigOption* o = g_new0(ConfigOption, 1);
	*o = (ConfigOption){
		.name = name,
		.save = save,
	};

	g_value_init(&o->val, G_TYPE_BOOLEAN);

	return o;
}


ConfigOption*
config_option_new_manual(void (*save)(ConfigOption*))
{
	ConfigOption* o = g_new0(ConfigOption, 1);
	*o = (ConfigOption){
		.save = save,
	};
	return o;
}

