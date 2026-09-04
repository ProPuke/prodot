/**************************************************************************/
/*  editor_script_tabs.h                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "scene/gui/margin_container.h"

class Button;
class HBoxContainer;
class MenuButton;
class Panel;
class PanelContainer;
class PopupMenu;
class TabBar;
class TextureRect;

class EditorScriptTabs : public MarginContainer {
	GDCLASS(EditorScriptTabs, MarginContainer);

	inline static EditorScriptTabs *singleton = nullptr;

public:
	enum {
		SCRIPT_NEW_SCRIPT,
		SCRIPT_NEW_TEXT,
		SCRIPT_SAVE,
		SCRIPT_SAVE_AS,
		SCRIPT_SAVE_ALL,
		SCRIPT_SHOW_IN_FILESYSTEM,
		SCRIPT_CLOSE,
		SCRIPT_CLOSE_OTHERS,
		SCRIPT_CLOSE_RIGHT,
		SCRIPT_CLOSE_ALL,
	};

private:
	bool _updating = false;

	PanelContainer *tabbar_panel = nullptr;
	HBoxContainer *tabbar_container = nullptr;

	TabBar *script_tabs = nullptr;
	PopupMenu *script_tabs_context_menu = nullptr;
	MenuButton *script_list = nullptr;
	Button *script_tab_add = nullptr;
	Control *script_tab_add_ph = nullptr;

	int last_hovered_tab = -1;

	void _script_tab_changed(int p_tab);
	void _script_tab_closed(int p_tab);
	void _scene_tab_input(const Ref<InputEvent> &p_input);
	void _script_tabs_resized();

	void _update_tab_titles();
	void _reposition_active_tab(int p_to_index);
	void _update_context_menu();
	void _custom_menu_option(int p_option);
	void _update_script_list();

	void _script_list_changed();

	void _new_script();
	void _new_text();

	void _menu_option(int p_option);

	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;

protected:
	void _notification(int p_what);

public:
	static EditorScriptTabs *get_singleton() { return singleton; }

	void init();

	void add_extra_button(Button *p_button);

	void set_current_tab(int p_tab);
	int get_current_tab() const;
	int get_option_tab() const;

	void update_script_tabs();

	EditorScriptTabs();
};
