#pragma once
#include "DiagramInterchange.hpp"
#include "tinyxml2.h"
#include <iostream>
#include <sstream>
/*
* Special Drawio-Value to DI-Member mappings:
* Drawio-Parent: gets resolved into the actual object and will be stored in owning_element
*/

/*
* Drawio-spezifischer Erweiterung des DI-Frameworks
*/
class DrawioMxcell : public DI::DiagramElement
{
public:
	std::unordered_map<std::string, std::string> drawio_style;
};

class DrawioArrow : public DI::Edge
{
public:
	std::unordered_map<std::string, std::string> drawio_style;
};


//anonymus namespace for helper functions
namespace
{

	/*
	* \brief Copy the given key if it exists
	*/
	bool copy_attr_if_exists(tinyxml2::XMLElement* from, DI::DiagramElement* to, std::string what)
	{
		if (from->Attribute(what.c_str()) == 0) return false;

		const std::string val = from->Attribute(what.c_str());
		to->local_style->properties.insert({ what, val});
		return true;
	}

	/*
	* \brief Copy the given key if it exists. Throw if not
	*/
	void copy_attr_or_throw(tinyxml2::XMLElement* from, DI::DiagramElement* to, std::string what)
	{
		if (!copy_attr_if_exists(from, to, what))
		{
			const std::string msg = '\'' + std::string(from->Value()) + "' does not contain attribute '" + what + '\'';
			throw std::logic_error(msg);
		}
	}

	/*
	* \brief Add child as owned element and set parent as owning element
	*/
	void set_relation(DI::DiagramElement* parent, DI::DiagramElement* child)
	{
		parent->owned_elements.push_back(child);
		child->owning_element = parent;
	}

	/*
	* \brief Find the first noyde wth matching key-value-Style. Depth first iteration
	* \param root - element to start looking from
	* \param key - key to search for
	* \param val - val to search for
	*/
	DI::DiagramElement* find_node_with(DI::DiagramElement* root, std::string key, std::string val)
	{
		// Check if the key exists and return the current node if the values match
		auto map = &root->local_style->properties;
		auto it_val = map->find(key);
		if (it_val != map->end())
		{
			std::string val_node = map->at(key);

			if (val_node == val) return root;
		}

		// recursion
		DI::DiagramElement* ret_val = nullptr;

		// loop through all childs. As soon as a match is found: return it.
		for (DI::DiagramElement* node : root->owned_elements)
		{
			ret_val = find_node_with(node, key, val);

			if (ret_val != nullptr) return ret_val;
		}

		// anchor
		return ret_val;
	}

	/*
	* \brief generic setup for mxCells (DrawioMxCell and DrawioArrow)
	* \param from: node to extract data from
	* \param to_mxcell: mxcell which will be polluted
	* \param root: root object to determine parent dependencies from
	* \param standard_parent: parent if none is specified in the xml
	*/
	void setup_mxcell(
		tinyxml2::XMLElement* from,
		DI::DiagramElement* to_mxcell,
		DI::DiagramElement* root,
		DI::DiagramElement* standard_parent)
	{
		// Required attributes
		copy_attr_or_throw(from, to_mxcell, "id");

		// other attributes
		if (from->Attribute("parent") != 0)
		{
			const std::string parent_id = from->Attribute("parent");
			/*if the file is valid the parent - object must already exist
			* resolve to parent object
			*/
			DI::DiagramElement* parent = find_node_with(root, "id", parent_id);
			set_relation(parent, to_mxcell);
		}
		else
		{
			set_relation(standard_parent, to_mxcell);
		}
	}


	/*
	* \brief Extract keys and values of a string.
	*		 Expects the following style: "key=value;key=value;"
	*/
	std::unordered_map<std::string, std::string> parse_style(const std::string& html_str)
	{
		std::unordered_map<std::string, std::string> map_ret;
		std::stringstream ss_semicolon{ html_str };
		// key value string
		std::string s_kv;

		while(std::getline(ss_semicolon, s_kv, ';'))
		{
			// pair_key_value should be like "key=value"
			std::stringstream ss_eval{ s_kv };

			try
			{
				std::string key;
				std::getline(ss_eval, key, '=');

				std::string value;
				std::getline(ss_eval, value, '=');

				map_ret.insert_or_assign(key, value);
			}
			catch (const std::exception& e)
			{
				const std::string msg = "semicolon sliced html_string fragment ''" + s_kv + "'' is not of the format ''key=value''.\nTrace:" + std::string(e.what());
				throw(std::logic_error(msg));
			}
			
		}
		return map_ret;
	}


	/*
	* \brief Iterate over all first level childs and pollute the diagram.
	*		 May call itself recursively if a first level child also has childs
	* \param base: root xml element
	* \param d_pollute: object to append nodes
	* \param root: must always be the root object of the tree
	*/
	bool iterate_children(tinyxml2::XMLElement* base, DI::DiagramElement* d_pollute, DI::DiagramElement* root)
	{
		for (tinyxml2::XMLElement* child = base->FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
		{
			// set a parent for the next recursive call of iterate_children
			// This value may be overwritten to the newly created node in this iteration
			// dont use this value as an r-value in any way except in the next iterate_children- call
			DI::DiagramElement* parent_next_iter = d_pollute;


			// information correctness:
			// if a value is requested it works in this order: local_style > global_style > specific member variable

			// GarphModel Object Construction (DI::DiagramElement)
			const std::string tag = child->Value();

			if (tag == "mxGraphModel")
			{
				DI::DiagramElement* graph = new DI::DiagramElement();

				// Required attrs
				copy_attr_or_throw(child, graph, "dx");
				copy_attr_or_throw(child, graph, "dy");
				copy_attr_or_throw(child, graph, "grid"); // this is a flag in UI
				copy_attr_or_throw(child, graph, "gridSize"); // TODO: does this exist if flag = 0?
				copy_attr_or_throw(child, graph, "guides");
				copy_attr_or_throw(child, graph, "tooltips");
				copy_attr_or_throw(child, graph, "connect");
				copy_attr_or_throw(child, graph, "arrows");
				copy_attr_or_throw(child, graph, "fold");
				copy_attr_or_throw(child, graph, "page");
				copy_attr_or_throw(child, graph, "pageScale");
				copy_attr_or_throw(child, graph, "pageWidth");
				copy_attr_or_throw(child, graph, "pageHeight");
				copy_attr_or_throw(child, graph, "math");
				copy_attr_or_throw(child, graph, "shadow");

				set_relation(d_pollute, graph);
				parent_next_iter = graph;
			}
			// Diagram Object Construction (DI::Diagram)
			else if (tag == "diagram")
			{
				DI::Diagram* diagram = new DI::Diagram();

				// Required
				copy_attr_or_throw(child, diagram, "id");
				copy_attr_or_throw(child, diagram, "name");

				// Set Parent-Relationship
				set_relation(d_pollute, diagram);

				parent_next_iter = diagram;

			}
			// Arrow Object Construction (DrawioArrow)
			else if (tag == "mxCell" &&
				(child->Attribute("source") != 0 || child->Attribute("target") != 0)) // check for arrow-indicators
			{
				DrawioArrow* arrow = new DrawioArrow;

				// Required attributes
				setup_mxcell(child, arrow, root, d_pollute);

				// temp. save target ID & source. This value will be resolved to pointers later.
				// because due to document structure these may not be in the tree yet
				copy_attr_or_throw(child, arrow, "source");
				copy_attr_or_throw(child, arrow, "target");
				copy_attr_or_throw(child, arrow, "edge"); // must map to "1"

				// parse style if it exists
				if (child->Attribute("style") != 0) arrow->drawio_style = parse_style(child->Attribute("style"));

				// if there is a recursive call: use this element as parent
				parent_next_iter = arrow;
			}
			// MxCell Object Construction (DrawioMxcell)
			else if (tag == "mxCell")
			{
				DrawioMxcell* cell = new DrawioMxcell();

				// Required attributes
				setup_mxcell(child, cell, root, d_pollute);

				// Other attributes
				copy_attr_if_exists(child, cell, "value"); // default label if drawio-attribute-injecion is false
				copy_attr_if_exists(child, cell, "vertex");
				// parse style if it exists
				if (child->Attribute("style") != 0) cell->drawio_style = parse_style(child->Attribute("style"));

				// if there is a recursive call: use this element as parent
				parent_next_iter = cell;
			}
			// mxGeometry Object Constructio (DI::DiagramElement)
			else if (tag == "mxGeometry")
			{
				DI::DiagramElement* geom = new DI::DiagramElement;

				// reqiuired attributes

				copy_attr_or_throw(child, geom, "as");

				// other attributes
				copy_attr_if_exists(child, geom, "x");
				copy_attr_if_exists(child, geom, "y");
				copy_attr_if_exists(child, geom, "width");
				copy_attr_if_exists(child, geom, "height");
				copy_attr_if_exists(child, geom, "relative");

				// mxGeometry has either x,y,w,h or relative

				d_pollute->owned_elements.push_back(geom);
			}
			//passthrough options for tag
			else if(tag == "root"){}
			//catch unexpected tags
			else 
			{
				const std::string msg = "Unexpected XML Tag '" + std::string(child->Value()) + "'.\n";
				throw std::logic_error(msg);
			}
			
			// if child also has childs -> call recursively with maybe-modified parent
			if (child->FirstChild() != nullptr) iterate_children(child, parent_next_iter, root);

		}
		return true;
	}

	bool iterate_resolve_arrows(DI::DiagramElement* parent, DI::DiagramElement* root)
	{
		for (DI::DiagramElement* node : parent->owned_elements)
		{
			// check if element is an arrow
			DrawioArrow* arrow = dynamic_cast<DrawioArrow*>(node);
			if (arrow != nullptr)
			{
				// resolve target & source
				const std::string id_target = arrow->local_style->properties.at("target");
				const std::string id_source = arrow->local_style->properties.at("source");

				DI::DiagramElement* target = find_node_with(root, "id", id_target);
				DI::DiagramElement* source = find_node_with(root, "id", id_source);

				// set values if they exist
				if (target != nullptr) arrow->target = target;
				else throw std::logic_error("Arrow sarget could not be resolved");

				if (target != nullptr) arrow->source = source;
				else throw std::logic_error("Arrow source could not be resolved");

				// remove keys to reduce redundancy;
				arrow->local_style->properties.erase("target");
				arrow->local_style->properties.erase("source");
			}

			// recursion looking for arrows
			if (node != nullptr && node->owned_elements.size() > 0) iterate_resolve_arrows(node, root);
		}
	}
}

/*
* Parse a drawio-File into DiagramInterchange
*/
bool parse_drawio_file(const std::string& path, DI::DiagramElement* d_pollute)
{
	// Init tinyxml2
	tinyxml2::XMLDocument doc;
	doc.LoadFile(path.c_str());
	
	tinyxml2::XMLElement* root = doc.RootElement();

	if (root == nullptr) return false;
	
	// iterate over all elements and pollute the passed DI::Diagram
	bool success = iterate_children(root, d_pollute, d_pollute);
	
	// late-resolve all DrawioArrows
	iterate_resolve_arrows(d_pollute, d_pollute);
	

	return success;
}

std::string generate_drawio_file(DI::Diagram* root)
{
	return "drawio file :D";
}