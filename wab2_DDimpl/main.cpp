#include "BijectiveAlgorithm.h"

#include "DiagramGraphics.hpp"
#include "DiagramInterChangeDrawio.hpp"
#include <fstream>
int id_get(const DI::DiagramElement* node)
{
	const auto& style = node->local_style.get()->properties;
	if (style.find("id") == style.end()) throw std::logic_error("value not present");
	try 
	{
		return std::stoi(style.at("id"));
	}
	catch(std::exception& e){}

	throw std::logic_error("value not present");
}

bool id_set(DI::DiagramElement* node, const int& value)
{
	// IDs sollen nicht modifizierbar sein.
	throw std::logic_error("value shall not be set");
	return false;
}

std::string value_get(const DI::DiagramElement* node)
{
	const auto& style = node->local_style.get()->properties;
	if (style.find("value") == style.end()) throw std::logic_error("value not present");

	return style.at("value");
}

std::string fillcolor_get(const DI::DiagramElement* node)
{
	const DrawioMxcell* node_mxcell = dynamic_cast<const DrawioMxcell*>(node);
	const DrawioArrow* node_arrow = dynamic_cast<const DrawioArrow*>(node);
	if(node_mxcell != nullptr)
	{
		const auto& style = node_mxcell->drawio_style;

		if(style.find("fillColor") == style.end()) throw std::logic_error("value not present");
		return style.at("fillColor");
	}
	else if(node_arrow != nullptr)
	{
		const auto& style = node_arrow->drawio_style;

		if (style.find("fillColor") == style.end()) throw std::logic_error("value not present");
		return style.at("fillColor");
	}

	throw std::logic_error("value not present");
}

bool fillcolor_set(DI::DiagramElement* node, const std::string& value)
{
	DrawioMxcell* node_mxcell = dynamic_cast<DrawioMxcell*>(node);
	DrawioArrow* node_arrow = dynamic_cast<DrawioArrow*>(node);
	if (node_mxcell != nullptr)
	{
		auto& style = node_mxcell->drawio_style;
		style.insert_or_assign("fillColor", value);
		return true;
	}
	else if (node_arrow != nullptr)
	{
		auto& style = node_arrow->drawio_style;
		style.insert_or_assign("fillColor", value);
		return true;
	}

	return false;
}

bool value_set(DI::DiagramElement* node, const std::string& value )
{
	auto& style = node->local_style.get()->properties;
	style.insert_or_assign("value", value);

	return true;
}

std::string fp_apply(std::string val)
{
	return "fake apply";
}

std::string fp_reverse(std::string val)
{
	return "fake reverse";
}

std::string vertex_get(const DI::DiagramElement* node)
{
	const auto& style = node->local_style.get()->properties;
	if (style.find("vertex") == style.end()) throw std::logic_error("value not present");

	return style.at("vertex");
}

//prevent editing vertex-flag
bool vertex_set(DI::DiagramElement* node, const std::string& value)
{
	return false;
}

std::vector<DI::DiagramElement*> func_get_childs(const DI::DiagramElement* node )
{
	if (node == nullptr) throw std::logic_error("Can't iterate over nullptr");

	const std::vector<DI::DiagramElement*> nodes = node->owned_elements;
	return nodes;
}

int main()
{
	// Lade eine Baumstruktur
	DI::Diagram tree;
	bool success = parse_drawio_file("test.drawio.xml", &tree);

	if (!success) {
		std::cout << "parsing error\n";
		return -1;
	}

	// Initialisiere Komponenten des BijectiveAlgorithm
	SelectiveView<DI::DiagramElement> view(func_get_childs);

	BijectiveAlgorithm<DI::DiagramElement> alg(view);

	// Initialisiere mehrere AttributePaths
	// Construct Path to all attributes we want to use
	auto p_id = alg.make_path<int>(id_get, id_set);

	auto p_value = alg.make_path<std::string>(value_get, value_set);
	p_value.default_value = std::optional<std::string>("");

	auto p_vertex = alg.make_path<std::string>(vertex_get, vertex_set);
	p_vertex.default_value = std::optional<std::string>("");

	auto p_fillcolor = alg.make_path<std::string>(fillcolor_get, fillcolor_set);

	// Initialisiere mehrere BijectiveModifier

	// Keine Wertemodifikation
	auto m_str_passthrough = BijeciveModifier<DI::DiagramElement, std::string, std::string>::passthrough([](const std::string& val) {return val; });

	// Teste die Verknüpfung mehrerer BijectiveModifier
	auto m_int_to_str = BijeciveModifier<DI::DiagramElement, int, std::string>(
		[](int i) {
			//return std::string("");
			return std::to_string(i);
		},
		[](const std::string& i)
		{
			//return 1;
			return std::stoi(i);
		}
	);

	auto m_int_add_one = BijeciveModifier<DI::DiagramElement, int, int>(
		[](int val)
		{
			return ++val;
		},
		[](int val)
		{
			return --val;
		}
		);

	auto chain = m_int_add_one.chain_with(m_int_to_str);

	// Initialisiere exportierbare Spalten
	auto col_id = alg.make_column(p_id, m_int_to_str, "ID-Spalte");

	auto col_val = alg.make_column(p_value, m_str_passthrough, "Label");

	auto col_fillcolor = alg.make_column(p_fillcolor, m_str_passthrough, "Farbe");

	// Initialisiere Filterobjekte
	auto f_vertex = new FilterAttributePath(p_vertex, { "1" });
	//auto f_fillcolor = new FilterAttributePath(p_fillcolor, { "#f8cecc" });

	// Filterkonfiguration
	// zu exportierende Knoten festlegen
	alg.view.apply_filter(&tree, { f_vertex });

	// modulare Spalten registrieren
	alg.register_column(col_id);
	alg.register_column(col_val);
	alg.register_column(col_fillcolor);

	// Spalte darstellen
	std::cout << "valid: " << alg.is_valid() << '\n';
	auto table = alg.apply();
	std::cout << "Table dump:\n";
	for (const std::vector<std::string>& data : table)
	{
		for (const std::string& val : data)
		{
			std::cout << '"' << val << "\"\t";
		}
		std::cout << '\n';
	}

	// Diese Werte sollen zur Aktualisierung verwendet werden
	std::vector<std::vector<std::string>> new_vals = {
		{"ID-Spalte", "3", "4", "6", "7"},
		{"Label", "E", "E", "G", "H" },
		{"Farbe", "E", "E", "G", "H" }
	};

	// Aktualisieren
	bool x = alg.sync_with(&tree, new_vals);
	std::cout << "sync success: " << x << '\n';

	std::cout << "valid: " << alg.is_valid() << '\n';
	
	// Aktualisierte Datenstruktur darstellen
	auto table_new = alg.apply();
	std::cout << "Table_new dump:\n";
	for (const std::vector<std::string>& data : table_new)
	{
		for (const std::string& val : data)
		{
			std::cout << '"' << val << "\"\t";
		}
		std::cout << '\n';
	}
}