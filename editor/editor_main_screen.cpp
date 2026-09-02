/**************************************************************************/
/*  editor_main_screen.cpp                                                */
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

#include "editor_main_screen.h"

#include "core/io/config_file.h"
#include "core/object/callable_mp.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"

void EditorMainScreen::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			set_accessibility_region(true);
			if (is_editor_enabled(EDITOR_3D)) {
				// If the 3D editor is enabled, use this as the default.
				select(EDITOR_3D);
				return;
			}

			// Switch to the first main screen plugin that is enabled. Usually this is
			// 2D, but may be subsequent ones if 2D is disabled in the feature profile.
			for (unsigned int i = 0; i < editors.size(); i++) {
				if (is_editor_enabled(i)) {
					select(i);
					return;
				}
			}

			select(-1);
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			{
				Ref<Texture2D> icon = get_theme_icon(SNAME("PackedScene"), EditorStringName(EditorIcons));
				if (icon.is_valid()) {
					scene_button->set_button_icon(icon);
				}
			}
			for (Editor &editor : editors) {
				if (editor.button == nullptr) {
					continue;
				}

				Ref<Texture2D> icon = editor.plugin->get_plugin_icon();

				if (icon.is_valid()) {
					editor.button->set_button_icon(icon);
				} else if (has_theme_icon(editor.plugin->get_plugin_name(), EditorStringName(EditorIcons))) {
					editor.button->set_button_icon(get_theme_icon(editor.plugin->get_plugin_name(), EditorStringName(EditorIcons)));
				}
			}
		} break;
	}
}

void EditorMainScreen::set_button_container(HBoxContainer *p_button_hb) {
	button_hb = p_button_hb;

	scene_button = memnew(Button);
	scene_button->set_toggle_mode(true);
	scene_button->set_theme_type_variation("MainScreenButton");
	scene_button->set_text(TTRC("Scene"));
	scene_button->set_shortcut(ED_SHORTCUT("editor/editor_scene", TTRC("Open Scene Workspace"), KeyModifierMask::CTRL | Key::F1, true));

	Ref<Texture2D> icon = get_theme_icon(SNAME("PackedScene"), EditorStringName(EditorIcons));
	if (icon.is_valid()) {
		scene_button->set_button_icon(icon);
		// Make sure the control is updated if the icon is reimported.
		icon->connect_changed(callable_mp((Control *)scene_button, &Control::update_minimum_size));
	}

	scene_button->connect(SceneStringName(pressed), callable_mp(this, &EditorMainScreen::select_scene_editor));

	button_hb->add_child(scene_button);
}

void EditorMainScreen::save_layout_to_config(Ref<ConfigFile> p_config_file, const String &p_section) const {
	int selected_main_editor_idx = -1;
	for (unsigned int i = 0; i < editors.size(); i++) {
		if (editors[i].button && editors[i].button->is_pressed()) {
			selected_main_editor_idx = i;
			break;
		}
	}
	if (selected_main_editor_idx != -1) {
		p_config_file->set_value(p_section, "selected_main_editor_idx", selected_main_editor_idx);
	} else {
		p_config_file->set_value(p_section, "selected_main_editor_idx", Variant());
	}
}

void EditorMainScreen::load_layout_from_config(Ref<ConfigFile> p_config_file, const String &p_section) {
	int selected_main_editor_idx = p_config_file->get_value(p_section, "selected_main_editor_idx", -1);
	if (selected_main_editor_idx >= 0 && selected_main_editor_idx < (int)editors.size()) {
		callable_mp(this, &EditorMainScreen::select).call_deferred(selected_main_editor_idx);
	}
}

void EditorMainScreen::set_editor_enabled(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)editors.size());

	editors[p_index].enabled = p_enabled;

	if (Button *button = editors[p_index].button) {
		button->set_visible(p_enabled);

		if (!p_enabled && button->is_pressed()) {
			select(EDITOR_2D);
		}
	}
}

bool EditorMainScreen::is_editor_enabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)editors.size(), false);
	return editors[p_index].enabled;
}

int EditorMainScreen::_get_current_main_editor() const {
	for (unsigned int i = 0; i < editors.size(); i++) {
		if (editors[i].plugin == selected_plugin) {
			return i;
		}
	}

	return 0;
}

void EditorMainScreen::select_next() {
	int editor = _get_current_main_editor();

	do {
		if (editor == (int)editors.size() - 1) {
			editor = 0;
		} else {
			editor++;
		}
	} while (!is_editor_enabled(editor));

	select(editor);
}

void EditorMainScreen::select_prev() {
	int editor = _get_current_main_editor();

	do {
		if (editor == 0) {
			editor = editors.size() - 1;
		} else {
			editor--;
		}
	} while (!is_editor_enabled(editor));

	select(editor);
}

void EditorMainScreen::select_by_name(const String &p_name) {
	ERR_FAIL_COND(p_name.is_empty());

	for (unsigned int i = 0; i < editors.size(); i++) {
		if (editors[i].button && editors[i].button->get_text() == p_name) {
			select(i);
			return;
		}
	}

	ERR_FAIL_MSG("The editor name '" + p_name + "' was not found.");
}

void EditorMainScreen::select(int p_index) {
	if (EditorNode::get_singleton()->is_changing_scene()) {
		return;
	}

	ERR_FAIL_INDEX(p_index, (int)editors.size());

	if (!is_editor_enabled(p_index)) {
		return;
	}

	for (unsigned int i = 0; i < editors.size(); i++) {
		if (editors[i].button) {
			editors[i].button->set_pressed_no_signal((int)i == p_index);
		}
	}

	EditorPlugin *new_editor = editors[p_index].plugin;
	ERR_FAIL_NULL(new_editor);

	if (selected_plugin == new_editor) {
		return;
	}

	if (selected_plugin) {
		selected_plugin->make_visible(false);
	}

	bool is_scene_editor = p_index == EDITOR_2D || p_index == EDITOR_3D;

	if (is_scene_editor) {
		last_scene_editor = (EditorTable)p_index;
	}
	
	scene_button->set_pressed(is_scene_editor);

	selected_plugin = new_editor;
	selected_plugin->make_visible(true);
	selected_plugin->selected_notify();
	set_accessibility_name(selected_plugin->get_plugin_name());

	EditorData &editor_data = EditorNode::get_editor_data();
	int plugin_count = editor_data.get_editor_plugin_count();
	for (int i = 0; i < plugin_count; i++) {
		editor_data.get_editor_plugin(i)->notify_main_screen_changed(selected_plugin->get_plugin_name());
	}

	EditorNode::get_singleton()->update_distraction_free_mode();
}

void EditorMainScreen::select_scene_editor() {
	select(last_scene_editor);
}

int EditorMainScreen::get_selected_index() const {
	for (unsigned int i = 0; i < editors.size(); i++) {
		if (selected_plugin == editors[i].plugin) {
			return i;
		}
	}
	return -1;
}

int EditorMainScreen::get_plugin_index(EditorPlugin *p_editor) const {
	int screen = -1;
	for (unsigned int i = 0; i < editors.size(); i++) {
		if (p_editor == editors[i].plugin) {
			screen = i;
			break;
		}
	}
	return screen;
}

EditorPlugin *EditorMainScreen::get_selected_plugin() const {
	return selected_plugin;
}

EditorPlugin *EditorMainScreen::get_plugin_by_name(const String &p_plugin_name) const {
	ERR_FAIL_COND_V(!main_editor_plugins.has(p_plugin_name), nullptr);
	return main_editor_plugins[p_plugin_name];
}

bool EditorMainScreen::can_auto_switch_screens() const {
	if (selected_plugin == nullptr) {
		return true;
	}
	// Only allow auto-switching if the selected button is to the left of the Script button.
	for (int i = 0; i < button_hb->get_child_count(); i++) {
		Button *button = Object::cast_to<Button>(button_hb->get_child(i));
		if (button->get_text() == "Script") {
			// Selected button is at or after the Script button.
			return false;
		}
		if (button->get_text() == selected_plugin->get_plugin_name()) {
			// Selected button is before the Script button.
			return true;
		}
	}
	return false;
}

VBoxContainer *EditorMainScreen::get_control() const {
	return main_screen_vbox;
}

void EditorMainScreen::add_main_plugin(EditorPlugin *p_editor) {
	bool is_scene_editor = editors.size() == EDITOR_2D || editors.size() == EDITOR_3D;

	bool add_button = !is_scene_editor;

	Button *tb = add_button ? memnew(Button) : nullptr;

	if (tb) {
		tb->set_toggle_mode(true);
		tb->set_theme_type_variation("MainScreenButton");
		tb->set_name(p_editor->get_plugin_name());
		tb->set_text(p_editor->get_plugin_name());

		Ref<Shortcut> shortcut = EditorSettings::get_singleton()->get_shortcut("editor/editor_" + p_editor->get_plugin_name().to_lower());
		if (shortcut.is_valid()) {
			tb->set_shortcut(shortcut);
		}

		Ref<Texture2D> icon = p_editor->get_plugin_icon();
		if (icon.is_null() && has_theme_icon(p_editor->get_plugin_name(), EditorStringName(EditorIcons))) {
			icon = get_editor_theme_icon(p_editor->get_plugin_name());
		}
		if (icon.is_valid()) {
			tb->set_button_icon(icon);
			// Make sure the control is updated if the icon is reimported.
			icon->connect_changed(callable_mp((Control *)tb, &Control::update_minimum_size));
		}

		tb->connect(SceneStringName(pressed), callable_mp(this, &EditorMainScreen::select).bind(editors.size()));

		button_hb->add_child(tb);
	}

	editors.push_back(Editor { tb, true, p_editor } );
	main_editor_plugins.insert(p_editor->get_plugin_name(), p_editor);
}

void EditorMainScreen::remove_main_plugin(EditorPlugin *p_editor) {
	// Unbind all buttons in advance (as indexes are about to change)
	for (Editor &editor : editors) {
		if (editor.button) {
			editor.button->disconnect(SceneStringName(pressed), callable_mp(this, &EditorMainScreen::select));
		}
	}

	int index = get_plugin_index(p_editor);
	if (index >= 0) {
		Button *button = editors[index].button;

		if (button) {
			if (button->is_pressed()) {
				select(EDITOR_SCRIPT);
			}
			memdelete(button);
			editors.remove_at(index);
		}

		editors.remove_at(index);
	}

	// Rebind buttons after with correct indexes
	for (unsigned int i = 0; i < editors.size(); i++) {
		if (editors[i].button) {
			editors[i].button->connect(SceneStringName(pressed), callable_mp(this, &EditorMainScreen::select).bind(i));
		}
	}

	main_editor_plugins.erase(p_editor->get_plugin_name());
}

EditorMainScreen::EditorMainScreen() {
	main_screen_vbox = memnew(VBoxContainer);
	main_screen_vbox->set_name("MainScreen");
	main_screen_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_screen_vbox->add_theme_constant_override("separation", 0);
	add_child(main_screen_vbox);
}
