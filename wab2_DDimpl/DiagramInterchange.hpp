#pragma once
#include "DiagramCommons.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

namespace DI
{
	//forwards declarations
	class Style;

	// Every MOF-Based Element
	class MOFBASE
	{

	};


	class DiagramElementVirtualizer
	{
	protected:
		virtual void foo() = 0;
	};

	class DiagramElement : public DiagramElementVirtualizer
	{
	private:
		void foo() {};
	public:

		~DiagramElement()
		{
			// delete all stored diagramElements
			for (DiagramElement* p : owned_elements)
			{
				delete p;
			}

			// Style will be deleted by strong-pointers
		}


		MOFBASE* md;//depiced model element TODO: what is this

		// Parent of this element
		DiagramElement* owning_element = nullptr;

		// Childs of this element
		std::vector<DiagramElement*> owned_elements = {};

		// IF there are properties overlapping with shared_style: loca_styles values are used first
		std::unique_ptr<Style> local_style = std::make_unique<Style>();

		// Optional, styls applied on this element
		std::shared_ptr<Style> shared_style;
	};



	// An Edge is rendered as a polyline
	// aka arrow
	class Edge : public DiagramElement
	{
	public:
		std::vector<Point> waypoints;

		DiagramElement* source;
		DiagramElement* target;
	};

	class Shape : public DiagramElement
	{
	public:
		//relative bounds to this objects nesting plane
		Bounds bounds;
	};


	class Diagram : public Shape
	{
	public:
		// Default values defined by spec
		std::string name = "";
		std::string documentation = "";
		double resolution = 300;
	};

	/*
	* Usage definition
	* cascading value on local style > cascading alue on shared style > cascading value in of the nearest DiagramElement (in case of parents) > default property value
	*/
	class Style
	{
	public:
		std::unordered_map<std::string, std::string> properties;
	};
}

