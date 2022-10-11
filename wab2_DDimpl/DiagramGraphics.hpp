#pragma once
#include "DiagramCommons.hpp"
#include <vector>

/*
* needed forward declarations
*/
namespace DG
{
	namespace abstracts
	{
		struct GraphicalElement;
	}
	struct Canvas;
	struct Group;
	struct ClipPath;
}


/*
* This segment contains the Style struct
*/
namespace DG
{
	/*
	* \brief See 10.3.32 (V1.1)
	*/
	struct Style
	{
		/*
		* \brief Properties may be not set. In this case value must not be accessed
		*/
		// TODO: make this std::optionals 
		template <typename T>
		struct Property
		{
			bool set = false;
			T value;
		};

		Property<Color> fillColor;
		Property<double> fillOpacity; // >= 0 && <= 1
		
		Property<double> strokeWidth; // >= 0
		Property<double> strokeOPacity;
		Property<Color> strokeColor;
		Property<std::vector<double>> strokeDashLength; //size must be even
		
		Property<double> fontSize; // >= 0
		Property<std::string> fontName;
		Property<Color> fontColor;
		Property<bool> fontItalic;
		Property<bool> fontBold;
		Property<bool> fontUnderline;
		Property<bool> fontStrikeThrough;
	};
}


/*
* This segment contains all defined data structures
*/
namespace DG
{
	/*
	******************
	* All PathCommands
	******************
	*/

	/*
	* \brief see 10.3.20 (V1.1)
	*/
	namespace abstracts
	{
		/*
		* Abstract Draw-Command
		*/
		struct PathCommand
		{
			bool relative = false;
		};
	}

	struct MoveTo : public abstracts::PathCommand
	{
		Point point;
	};

	/*
	* \brief draw a cubic bézier curve from the current point to the given point
	*/
	struct CubicCurveTo : public abstracts::PathCommand
	{
		Point point;
		Point control_start, control_end;
	};

	struct LineTo : public abstracts::PathCommand
	{
		Point point;
	};

	struct QuadraticCurveTo : public abstracts::PathCommand
	{
		Point point;
		Point control;
	};

	/*
	* \brief end the current subpath and draw a straight line from the current position to the initial point of the subpath
	*/
	struct ClosePath : public abstracts::PathCommand {};

	struct EllipticalArcTo
	{
		Point point;
		Dimension radii;
		double rotation;
		bool large_arc;
		bool sweep;
	};

	/*
	*****************
	* Transform Types
	*****************
	*/
	namespace abstracts
	{
		/*
		* \brief change the geometry of a graphical element in some way
		*/
		struct Transform {}; // may be hit by empty basestruct optimization?
	}

	/*
	* \brief A matrix with a fixed dimension (3x3), only values in the first 2 rows are accessible. (row3 = 0,0,1)
	* The following implementation layout is defined by the spec :D
	*/
	struct Matrix : public abstracts::Transform
	{
		union
		{
			double _array[6];
			double a, b, c, d, e, f;
		};

		// As defined in diagram 10.14 (V1.1)
		Point operator*(const Point& coordinates) const noexcept
		{
			Point p;
			p.x = (coordinates.x * a) + (coordinates.x * b) + (coordinates.x * c);
			p.y = (coordinates.y * d) + (coordinates.y * e) + (coordinates.y * f);

			return p;
		}
	};

	struct Translate : public abstracts::Transform
	{
		double x_delta, y_delta;
	};

	struct Scale : public abstracts::Transform
	{
		double x_factor, y_factor;
	};

	struct Rotate : public abstracts::Transform
	{
		double angle;
		Point center;
	};

	struct Skew : public abstracts::Transform
	{
		double x_angle, y_angle;
	};

	/*
	***********
	* Gradients
	***********
	*/


	struct GradientStop
	{
		Color color;

		/*
		* bot between [0 and 1] 0=transparent, 1=opaque
		*/
		double offset, opacity;
	};
}


/*
* This segment contains all Gradient/Fill structes
*/
namespace DG
{
	namespace abstracts
	{
		/*
		* Paint the enterior of a graphical element
		*/
		struct Fill
		{
			std::vector<Transform*> transforms;

			Canvas* owner = nullptr;
		};


		/*
		* Paint a smooth color transition
		*/
		struct Gradient : public Fill
		{
			//TODO: must be 2 or more
			std::vector<GradientStop> stops;
		};
	}

	/*
	* 10.3.16 (V1.1)
	*/
	struct LinearGradient : public abstracts::Gradient
	{
		/*
		* Between [0 and 1]
		*/
		double x1, x2, y1, y2;
	};


	struct RadialGradient : public abstracts::Gradient
	{
		/*
		* Between [0 and 1]
		*/
		double x_center = 0.5, y_center = 0.5;
		double radius = 0.5;
		double x_focus = 0.5, y_focus = 0.5;
	};


	/*
	* Draws a single tile repeatedly to fill an area
	*/
	struct Pattern : public abstracts::Fill
	{
		Bounds bounds;

		abstracts::GraphicalElement* tile = nullptr;
	};
}


/*
* This segment contains Group and Graphical Element structes
*/
namespace DG
{
	namespace abstracts
	{
		/* 
		* The following function is not defined in the spec.
		* Its used for allowing dynamic upcasts of GraphicalElement-Derived structes. This is only possible for C++ polynomial Types 
		* which require at least one virtual function
		*/
		struct GraphicalElementVirtualizer
		{
		public:
			virtual void foo();
		};


		struct GraphicalElement : public GraphicalElementVirtualizer
		{
			std::vector<abstracts::Transform*> transforms;

			Group* owner = nullptr;

			// Styles
			std::vector<Style> s_local;
			std::vector<Style*> s_shared;

			ClipPath* mask = nullptr;

			void foo() {};
		};
	}

	/*
	* \brief Group of graphical elements
	*
	* - Owned graphical elements are drawn above their parent
	* - elements appearing earlier in the container are drawn above elements appearing later
	*/
	struct Group : public abstracts::GraphicalElement
	{
		std::vector<abstracts::GraphicalElement*> members;
	};

	/*
	* \brief Defines an arrowhead
	*/



	struct ClipPath : public Group
	{
		// TODO: what does it do?
		abstracts::GraphicalElement* owner;
	};


	/*
	* \brief root object for all graphical elements
	*/
	struct Canvas : public Group
	{
		// exclusive, if c_background and f_background are set f_background is used
		// no valid value evaluates to transparent
		Color c_background;
		abstracts::Fill* f_background = nullptr;

		// All fills this canvas ownes
		// new Fill-Subtypes are stored here
		std::vector<abstracts::Fill*> package_fills;

		std::vector<Style> package_styles;
	};



	struct Rectangle : public abstracts::GraphicalElement
	{
	public:
		Bounds bounds;

		double corner_radius = 0;
	};



	struct Circle : public abstracts::GraphicalElement
	{
		Point center;

		double radius;
	};

	struct Ellipse
	{
		Point center;

		Dimension radii;
	};

	struct Text : public abstracts::GraphicalElement
	{
		std::string data;
		Bounds bounds;
		AlignmentKind alignment;
	};

	struct Image
	{
		std::string source;
		Bounds bounds;
		bool is_aspectRatio_preserved;
	};
}


/*
* This segment contains all Marker structes
*/
namespace DG
{
	struct Marker : public Group
	{
		Dimension size;

		// target the arrow points to
		Point reference;

		Canvas* owner;
	};

	namespace abstracts
	{

		/*
		* Object which can be decorated with markers
		*/
		struct MarkedElement : public GraphicalElement
		{
			Marker* start = nullptr, * end = nullptr, * mid = nullptr;
		};
	}

	/*
	* \brief Simple line
	*/
	struct Line : public abstracts::MarkedElement
	{
		Point start, end;
	};

	/*
	* A complex object drawn via the stored commands
	*/
	struct Path : public abstracts::MarkedElement
	{
		std::vector<abstracts::PathCommand*> commands;
	};

	/*
	* This object can be drawn with 3 or more lines
	*/
	struct Polygon : public abstracts::MarkedElement
	{
		std::vector<Point> points; // TODO. must be >= 3 Points
	};

	/*
	* Thos object can be drawn with one or more line
	*/
	struct PolyLine : public abstracts::MarkedElement
	{
		std::vector<Point> points; //TODO: must be >= 2 Points
	};
}
