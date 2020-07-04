/*************************************************************************/
/*  editor_translation_parser.cpp                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "editor_translation_parser.h"

#include "core/error_macros.h"
#include "core/os/file_access.h"
#include "core/script_language.h"
#include "core/set.h"

EditorTranslationParser *EditorTranslationParser::singleton = nullptr;

Error EditorTranslationParserPlugin::parse_file(const String &p_path, Vector<String> *r_extracted_strings) {
	if (!get_script_instance())
		return ERR_UNAVAILABLE;

	if (get_script_instance()->has_method("parse_text")) {
		Error err;
		FileAccess *file = FileAccess::open(p_path, FileAccess::READ, &err);
		if (err != OK) {
			ERR_PRINT("Failed to open " + p_path);
			return err;
		}
		parse_text(file->get_as_utf8_string(), r_extracted_strings);
		return OK;
	} else {
		ERR_PRINT("Custom translation parser plugin's \"func parse_text(text, extracted_strings)\" is undefined.");
		return ERR_UNAVAILABLE;
	}
}

void EditorTranslationParserPlugin::parse_text(const String &p_text, Vector<String> *r_extracted_strings) {
	if (!get_script_instance())
		return;

	if (get_script_instance()->has_method("parse_text")) {
		Array extracted_strings;
		get_script_instance()->call("parse_text", p_text, extracted_strings);
		for (int i = 0; i < extracted_strings.size(); i++) {
			r_extracted_strings->append(extracted_strings[i]);
		}
	} else {
		ERR_PRINT("Custom translation parser plugin's \"func parse_text(text, extracted_strings)\" is undefined.");
	}
}

void EditorTranslationParserPlugin::get_recognized_extensions(List<String> *r_extensions) const {
	if (!get_script_instance())
		return;

	if (get_script_instance()->has_method("get_recognized_extensions")) {
		Array extensions = get_script_instance()->call("get_recognized_extensions");
		for (int i = 0; i < extensions.size(); i++) {
			r_extensions->push_back(extensions[i]);
		}
	} else {
		ERR_PRINT("Custom translation parser plugin's \"func get_recognized_extensions()\" is undefined.");
	}
}

void EditorTranslationParserPlugin::_bind_methods() {
	ClassDB::add_virtual_method(get_class_static(), MethodInfo(Variant::NIL, "parse_text", PropertyInfo(Variant::STRING, "text"), PropertyInfo(Variant::ARRAY, "extracted_strings")));
	ClassDB::add_virtual_method(get_class_static(), MethodInfo(Variant::ARRAY, "get_recognized_extensions"));
}

/////////////////////////

void EditorTranslationParser::get_recognized_extensions(List<String> *r_extensions) const {
	Set<String> extensions;
	List<String> temp;
	for (int i = 0; i < standard_parsers.size(); i++) {
		standard_parsers[i]->get_recognized_extensions(&temp);
	}
	for (int i = 0; i < custom_parsers.size(); i++) {
		custom_parsers[i]->get_recognized_extensions(&temp);
	}
	// Remove duplicates.
	for (int i = 0; i < temp.size(); i++) {
		extensions.insert(temp[i]);
	}
	for (auto E = extensions.front(); E; E = E->next()) {
		r_extensions->push_back(E->get());
	}
}

bool EditorTranslationParser::can_parse(const String &p_extension) const {
	List<String> extensions;
	get_recognized_extensions(&extensions);
	for (int i = 0; i < extensions.size(); i++) {
		if (p_extension == extensions[i]) {
			return true;
		}
	}
	return false;
}

Ref<EditorTranslationParserPlugin> EditorTranslationParser::get_parser(const String &p_extension) const {
	// Consider user-defined parsers first.
	for (int i = 0; i < custom_parsers.size(); i++) {
		List<String> temp;
		custom_parsers[i]->get_recognized_extensions(&temp);
		for (int j = 0; j < temp.size(); j++) {
			if (temp[j] == p_extension) {
				return custom_parsers[i];
			}
		}
	}

	for (int i = 0; i < standard_parsers.size(); i++) {
		List<String> temp;
		standard_parsers[i]->get_recognized_extensions(&temp);
		for (int j = 0; j < temp.size(); j++) {
			if (temp[j] == p_extension) {
				return standard_parsers[i];
			}
		}
	}

	WARN_PRINT("No translation parser available for \"" + p_extension + "\" extension.");

	return nullptr;
}

void EditorTranslationParser::add_parser(const Ref<EditorTranslationParserPlugin> &p_parser, ParserType p_type) {
	if (p_type == ParserType::STANDARD) {
		standard_parsers.push_back(p_parser);
	} else if (p_type == ParserType::CUSTOM) {
		custom_parsers.push_back(p_parser);
	}
}

void EditorTranslationParser::remove_parser(const Ref<EditorTranslationParserPlugin> &p_parser, ParserType p_type) {
	if (p_type == ParserType::STANDARD) {
		standard_parsers.erase(p_parser);
	} else if (p_type == ParserType::CUSTOM) {
		custom_parsers.erase(p_parser);
	}
}

EditorTranslationParser *EditorTranslationParser::get_singleton() {
	if (!singleton) {
		singleton = memnew(EditorTranslationParser);
	}
	return singleton;
}

EditorTranslationParser::EditorTranslationParser() {
}

EditorTranslationParser::~EditorTranslationParser() {
	memdelete(singleton);
	singleton = nullptr;
}