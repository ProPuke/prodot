/**************************************************************************/
/*  editor_script_tabs.cpp                                                */
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

#include "editor_script_tabs.h"

#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/docks/inspector_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/run/editor_run_bar.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/script/script_text_editor.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/tab_bar.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/texture_rect.h"

void EditorScriptTabs::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			tabbar_panel->add_theme_style_override(SceneStringName(panel), get_theme_stylebox(SNAME("tabbar_background"), SNAME("TabContainer")));
			script_tabs->add_theme_constant_override("icon_max_width", get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor)));

			script_list->set_button_icon(get_editor_theme_icon(SNAME("GuiTabMenuHl")));
			_update_tab_titles();

			script_tab_add->set_button_icon(get_editor_theme_icon(SNAME("Add")));
			script_tab_add->add_theme_color_override("icon_normal_color", Color(0.6f, 0.6f, 0.6f, 0.8f));

			script_tab_add_ph->set_custom_minimum_size(script_tab_add->get_minimum_size());
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/scene_tabs")) {
				script_tabs->set_tab_close_display_policy((TabBar::CloseButtonDisplayPolicy)EDITOR_GET("interface/scene_tabs/display_close_button").operator int());
				script_tabs->set_max_tab_width(int(EDITOR_GET("interface/scene_tabs/maximum_width")) * EDSCALE);
				_script_tabs_resized();
			}
		} break;

		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
		case NOTIFICATION_TRANSLATION_CHANGED: {
			_update_tab_titles();
		} break;
	}
}

void EditorScriptTabs::_script_tab_changed(int p_tab) {
	if (_updating) {
		return;
	}
	ScriptEditor::get_singleton()->script_list->select(p_tab);
	ScriptEditor::get_singleton()->script_list->emit_signal(SceneStringName(item_selected), p_tab);
}

void EditorScriptTabs::_script_tab_closed(int p_tab) {
	ScriptEditor::get_singleton()->_menu_option(ScriptEditor::FILE_MENU_CLOSE);
}

void EditorScriptTabs::_scene_tab_input(const Ref<InputEvent> &p_input) {
	Ref<InputEventMouseButton> mb = p_input;

	if (mb.is_valid()) {
		if (script_tabs->get_hovered_tab() < 0 && mb->get_button_index() == MouseButton::LEFT && mb->is_double_click()) {
			int tab_buttons = 0;
			if (script_tabs->get_offset_buttons_visible()) {
				tab_buttons = get_theme_icon(SNAME("increment"), SNAME("TabBar"))->get_width() + get_theme_icon(SNAME("decrement"), SNAME("TabBar"))->get_width();
			}

			if ((is_layout_rtl() && mb->get_position().x > tab_buttons) || (!is_layout_rtl() && mb->get_position().x < script_tabs->get_size().width - tab_buttons)) {
				_new_script();
			}
		} else if (mb->get_button_index() == MouseButton::RIGHT && mb->is_pressed()) {
			// Context menu.
			_update_context_menu();

			script_tabs_context_menu->set_position(script_tabs->get_screen_position() + mb->get_position());
			script_tabs_context_menu->reset_size();
			script_tabs_context_menu->popup();
		}
	}
}

void EditorScriptTabs::_reposition_active_tab(int p_to_index) {
	ScriptEditor *editor = ScriptEditor::get_singleton();
	int index = editor->script_list->get_current();

	editor->tab_container->move_child(editor->tab_container->get_child(index, false), p_to_index);
	editor->tab_container->set_current_tab(p_to_index);

	update_script_tabs();
}

void EditorScriptTabs::_update_context_menu() {
#define DISABLE_LAST_OPTION_IF(m_condition) \
	if (m_condition) { \
		script_tabs_context_menu->set_item_disabled(-1, true); \
	}

	script_tabs_context_menu->clear();
	script_tabs_context_menu->reset_size();

	int tab_id = script_tabs->get_hovered_tab();

	script_tabs_context_menu->add_shortcut(ED_SHORTCUT("editor/new_script", TTRC("New Script..."), KeyModifierMask::CMD_OR_CTRL | Key::N), SCRIPT_NEW_SCRIPT);
	script_tabs_context_menu->add_shortcut(ED_SHORTCUT("editor/new_textfile", TTRC("New Text File..."), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::N), SCRIPT_NEW_TEXT);
	if (tab_id >= 0) {
		script_tabs_context_menu->add_shortcut(ED_SHORTCUT("editor/save_script", TTRC("Save")), SCRIPT_SAVE);
		script_tabs_context_menu->add_shortcut(ED_SHORTCUT("editor/save_script_as", TTRC("Save As...")), SCRIPT_SAVE_AS);
	}

	bool has_unsaved_scripts = !ScriptEditor::get_singleton()->script_editor->get_unsaved_scripts().is_empty();
	script_tabs_context_menu->add_shortcut(ED_SHORTCUT("editor/save_all_scripts", TTRC("Save All"), KeyModifierMask::SHIFT | KeyModifierMask::ALT | Key::S), SCRIPT_SAVE_ALL);
	ED_SHORTCUT_OVERRIDE("editor/save_all_scripts", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::S);
	DISABLE_LAST_OPTION_IF(!has_unsaved_scripts);

	if (tab_id >= 0) {
		const String scene_path = EditorNode::get_editor_data().get_scene_path(tab_id);

		script_tabs_context_menu->add_separator();
		script_tabs_context_menu->add_item(TTR("Show in FileSystem"), SCRIPT_SHOW_IN_FILESYSTEM);
		DISABLE_LAST_OPTION_IF(!ResourceLoader::exists(scene_path));

		script_tabs_context_menu->add_separator();
		script_tabs_context_menu->add_shortcut(ED_SHORTCUT("editor/close_script", TTRC("Close Script")), SCRIPT_CLOSE);
		script_tabs_context_menu->set_item_text(-1, TTR("Close Tab"));
		// script_tabs_context_menu->add_shortcut(ED_SHORTCUT("editor/reopen_closed_script", TTRC("Reopen Closed Script")), SCRIPT_OPEN_PREV);
		// script_tabs_context_menu->set_item_text(-1, TTR("Undo Close Tab"));
		// DISABLE_LAST_OPTION_IF(!EditorNode::get_singleton()->has_previous_closed_scenes());
		script_tabs_context_menu->add_item(TTR("Close Other Tabs"), SCRIPT_CLOSE_OTHERS);
		DISABLE_LAST_OPTION_IF(EditorNode::get_editor_data().get_edited_scene_count() <= 1);
		script_tabs_context_menu->add_item(TTR("Close Tabs to the Right"), SCRIPT_CLOSE_RIGHT);
		DISABLE_LAST_OPTION_IF(EditorNode::get_editor_data().get_edited_scene_count() == tab_id + 1);
		script_tabs_context_menu->add_shortcut(ED_SHORTCUT("editor/close_all_scripts", TTRC("Close All Scripts")), SCRIPT_CLOSE_ALL);
		script_tabs_context_menu->set_item_text(-1, TTRC("Close All Tabs"));

		const PackedStringArray paths = { EditorNode::get_editor_data().get_scene_path(tab_id) };
		EditorContextMenuPluginManager::get_singleton()->add_options_from_plugins(script_tabs_context_menu, EditorContextMenuPlugin::CONTEXT_SLOT_SCRIPT_EDITOR_CODE, paths);
	} else {
		script_tabs_context_menu->add_separator();
		// script_tabs_context_menu->add_shortcut(ED_SHORTCUT("editor/reopen_closed_script", TTRC("Reopen Closed Script")), SCRIPT_OPEN_PREV);
		// script_tabs_context_menu->set_item_text(-1, TTRC("Undo Close Tab"));
		// DISABLE_LAST_OPTION_IF(!EditorNode::get_singleton()->has_previous_closed_scenes());
		script_tabs_context_menu->add_shortcut(ED_SHORTCUT("editor/close_all_scripts", TTRC("Close All Scripts")), SCRIPT_CLOSE_ALL);
		script_tabs_context_menu->set_item_text(-1, TTRC("Close All Tabs"));

		EditorContextMenuPluginManager::get_singleton()->add_options_from_plugins(script_tabs_context_menu, EditorContextMenuPlugin::CONTEXT_SLOT_SCRIPT_EDITOR_CODE, {});
	}
#undef DISABLE_LAST_OPTION_IF

	last_hovered_tab = tab_id;
}

int EditorScriptTabs::get_option_tab() const {
	return last_hovered_tab >= 0 ? last_hovered_tab : script_tabs->get_current_tab();
}

void EditorScriptTabs::_custom_menu_option(int p_option) {
	if (p_option >= EditorContextMenuPlugin::BASE_ID) {
		if (last_hovered_tab >= 0) {
			ScriptTextEditor *ste = Object::cast_to<ScriptTextEditor>(ScriptEditor::get_singleton()->tab_container->get_tab_control(last_hovered_tab));
			if (ste) {
				EditorContextMenuPluginManager::get_singleton()->activate_custom_option(EditorContextMenuPlugin::CONTEXT_SLOT_SCRIPT_EDITOR_CODE, p_option, ste->get_code_editor()->get_text_editor());
			}
		}
	}
}

void EditorScriptTabs::_update_script_list() {
	PopupMenu *popup = script_list->get_popup();
	popup->clear();

	for (int i = 0; i < script_tabs->get_tab_count(); i++) {
		popup->add_item(script_tabs->get_tab_title(i), i);
		popup->set_item_icon(i, script_tabs->get_tab_icon(i));
	}
}

void EditorScriptTabs::update_script_tabs() {
	_update_tab_titles();
}

void EditorScriptTabs::_update_tab_titles() {
	if (_updating) {
		return;
	}
	_updating = true;
	ScriptEditor *editor = ScriptEditor::get_singleton();

	int count = editor->script_list->get_item_count();
	Ref<Texture2D> icon = get_editor_theme_icon(SNAME("Script"));
	script_tabs->set_tab_count(count);
	for (int i = 0; i < count; ++i) {
		script_tabs->set_tab_title(i, editor->script_list->get_item_text(i));
		script_tabs->set_tab_icon(i, editor->script_list->get_item_icon(i));
	}

	int current_tab = editor->script_list->get_current();
	if (current_tab >= 0 && script_tabs->get_tab_count() > 0 && script_tabs->get_current_tab() != current_tab) {
		script_tabs->set_block_signals(true);
		script_tabs->set_current_tab(current_tab);
		script_tabs->set_block_signals(false);
	}

	_script_tabs_resized();
	_updating = false;
}

void EditorScriptTabs::_script_tabs_resized() {
	const Size2 add_button_size = Size2(script_tab_add->get_size().x, script_tabs->get_size().y);
	if (script_tabs->get_offset_buttons_visible()) {
		// Move the add button to a fixed position.
		if (script_tab_add->get_parent() == script_tabs) {
			script_tabs->remove_child(script_tab_add);
			script_tab_add_ph->add_child(script_tab_add);
			script_tab_add->set_rect(Rect2(Point2(), add_button_size));
		}
	} else {
		// Move the add button to be after the last tab.
		if (script_tab_add->get_parent() == script_tab_add_ph) {
			script_tab_add_ph->remove_child(script_tab_add);
			script_tabs->add_child(script_tab_add);
		}

		if (script_tabs->get_tab_count() == 0) {
			script_tab_add->set_rect(Rect2(Point2(), add_button_size));
			return;
		}

		Rect2 last_tab = script_tabs->get_tab_rect(script_tabs->get_tab_count() - 1);
		int hsep = script_tabs->get_theme_constant(SNAME("h_separation"));
		if (script_tabs->is_layout_rtl()) {
			script_tab_add->set_rect(Rect2(Point2(last_tab.position.x - add_button_size.x - hsep, last_tab.position.y), add_button_size));
		} else {
			script_tab_add->set_rect(Rect2(Point2(last_tab.position.x + last_tab.size.width + hsep, last_tab.position.y), add_button_size));
		}
	}
}

void EditorScriptTabs::shortcut_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventKey> k = p_event;
	if ((k.is_valid() && k->is_pressed() && !k->is_echo()) || Object::cast_to<InputEventShortcut>(*p_event)) {
		if (ED_IS_SHORTCUT("editor/next_tab", p_event)) {
			TabContainer *editor_tabs = ScriptEditor::get_singleton()->tab_container;
			int next_tab = editor_tabs->get_current_tab() + 1;
			next_tab %= editor_tabs->get_tab_count();
			set_current_tab(next_tab);
		}
		if (ED_IS_SHORTCUT("editor/prev_tab", p_event)) {
			TabContainer *editor_tabs = ScriptEditor::get_singleton()->tab_container;
			int next_tab = editor_tabs->get_current_tab() - 1;
			next_tab = next_tab >= 0 ? next_tab : editor_tabs->get_tab_count() - 1;
			set_current_tab(next_tab);
		}
	}
}

void EditorScriptTabs::add_extra_button(Button *p_button) {
	tabbar_container->add_child(p_button);
}

void EditorScriptTabs::set_current_tab(int p_tab) {
	script_tabs->set_current_tab(p_tab);
}

int EditorScriptTabs::get_current_tab() const {
	return script_tabs->get_current_tab();
}

void EditorScriptTabs::_script_list_changed() {
	_update_tab_titles();
}

void EditorScriptTabs::_new_script() {
	ScriptEditor::get_singleton()->open_new_script_dialog();
}

void EditorScriptTabs::_new_text() {
	ScriptEditor::get_singleton()->open_new_text_dialog();
}

void EditorScriptTabs::_menu_option(int p_option) {
	switch (p_option) {
		case SCRIPT_NEW_SCRIPT:
			_new_script();
			break;
		case SCRIPT_NEW_TEXT:
			_new_text();
			break;
		case SCRIPT_SAVE:
			ScriptEditor::get_singleton()->_menu_option(ScriptEditor::FILE_MENU_SAVE);
			break;
		case SCRIPT_SAVE_AS:
			ScriptEditor::get_singleton()->_menu_option(ScriptEditor::FILE_MENU_SAVE_AS);
			break;
		case SCRIPT_SAVE_ALL:
			ScriptEditor::get_singleton()->_menu_option(ScriptEditor::FILE_MENU_SAVE_ALL);
			break;
		case SCRIPT_SHOW_IN_FILESYSTEM:
			ScriptEditor::get_singleton()->_menu_option(ScriptEditor::FILE_MENU_SHOW_IN_FILE_SYSTEM);
			break;
		case SCRIPT_CLOSE:
			ScriptEditor::get_singleton()->_menu_option(ScriptEditor::FILE_MENU_CLOSE);
			break;
		case SCRIPT_CLOSE_OTHERS:
			ScriptEditor::get_singleton()->_menu_option(ScriptEditor::FILE_MENU_CLOSE_OTHER_TABS);
			break;
		case SCRIPT_CLOSE_RIGHT:
			ScriptEditor::get_singleton()->_menu_option(ScriptEditor::FILE_MENU_CLOSE_TABS_BELOW);
			break;
		case SCRIPT_CLOSE_ALL:
			ScriptEditor::get_singleton()->_menu_option(ScriptEditor::FILE_MENU_CLOSE_ALL);
			break;
	}
}

void EditorScriptTabs::init() {
	ScriptEditor *editor = ScriptEditor::get_singleton();

	editor->script_list->connect(CoreStringName(property_list_changed), callable_mp(this, &EditorScriptTabs::_script_list_changed), CONNECT_DEFERRED);
}

EditorScriptTabs::EditorScriptTabs() {
	singleton = this;

	set_process_shortcut_input(true);
	set_process_unhandled_key_input(true);

	tabbar_panel = memnew(PanelContainer);
	add_child(tabbar_panel);
	tabbar_container = memnew(HBoxContainer);
	tabbar_panel->add_child(tabbar_container);

	script_tabs = memnew(TabBar);
	script_tabs->add_tab("unsaved");
	script_tabs->set_tab_close_display_policy((TabBar::CloseButtonDisplayPolicy)EDITOR_GET("interface/scene_tabs/display_close_button").operator int());
	script_tabs->set_max_tab_width(int(EDITOR_GET("interface/scene_tabs/maximum_width")) * EDSCALE);
	script_tabs->set_drag_to_rearrange_enabled(true);
	script_tabs->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	script_tabs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tabbar_container->add_child(script_tabs);

	script_tabs->connect("tab_changed", callable_mp(this, &EditorScriptTabs::_script_tab_changed));
	script_tabs->connect("tab_close_pressed", callable_mp(this, &EditorScriptTabs::_script_tab_closed));
	script_tabs->connect(SceneStringName(gui_input), callable_mp(this, &EditorScriptTabs::_scene_tab_input));
	script_tabs->connect("active_tab_rearranged", callable_mp(this, &EditorScriptTabs::_reposition_active_tab));
	script_tabs->connect(SceneStringName(resized), callable_mp(this, &EditorScriptTabs::_script_tabs_resized), CONNECT_DEFERRED);

	script_tabs_context_menu = memnew(PopupMenu);
	tabbar_container->add_child(script_tabs_context_menu);
	script_tabs_context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorScriptTabs::_menu_option));
	script_tabs_context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorScriptTabs::_custom_menu_option));

	script_tab_add = memnew(Button);
	script_tab_add->set_flat(true);
	script_tab_add->set_tooltip_text(TTR("Add a new script."));
	script_tabs->add_child(script_tab_add);
	script_tab_add->connect(SceneStringName(pressed), callable_mp(this, &EditorScriptTabs::_new_script));

	script_tab_add_ph = memnew(Control);
	script_tab_add_ph->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	script_tab_add_ph->set_custom_minimum_size(script_tab_add->get_minimum_size());
	tabbar_container->add_child(script_tab_add_ph);

	script_list = memnew(MenuButton);
	script_list->set_flat(false);
	script_list->set_theme_type_variation("FlatMenuButton");
	script_list->set_accessibility_name(TTRC("Show Opened Scripts List"));
	script_list->set_shortcut(ED_SHORTCUT("editor/show_opened_scripts_list", TTRC("Show Opened Scripts List"), KeyModifierMask::ALT | Key::T));
	script_list->get_popup()->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	script_list->get_popup()->set_search_bar_enabled(true);
	script_list->get_popup()->set_search_bar_min_item_count(10);
	script_list->get_popup()->connect("about_to_popup", callable_mp(this, &EditorScriptTabs::_update_script_list));
	script_list->get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &EditorScriptTabs::set_current_tab));
	tabbar_container->add_child(script_list);

	// On-hover tab preview.

	Control *tab_preview_anchor = memnew(Control);
	tab_preview_anchor->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	add_child(tab_preview_anchor);
}
