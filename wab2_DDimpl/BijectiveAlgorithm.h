#pragma once
#include <string>
#include <functional>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <optional>

// TODO: only needed for debugging. Must be removed in the end
#include <iostream>

/*
* How to use this file
* -> Build AttributePaths to attributes you want to use
* -> Declare AttributeFilters containing a Path and values it should return
*	-> Use AttributeFilters to pollute a SelectiveView
* -> Build BijectiveModifer
*	-> Must be at least AttributePath-Value -> std::string
*	-> can contain more steps, must be chained in this case
* -> Build Columns using AttributePaths and BijectiveModifer
* -> Init Algorithm with a view and columns, check if it is valid and run
*/


/*
* Contains a function resolving a DiagramElement to some value T
*/
template<typename T_node, typename T_out>
class AttributePath
{
public:
	std::optional<T_out> default_value;
	std::function<T_out(const T_node*)> func_get;
	std::function<bool(T_node*, const T_out& val)> func_set;

	// Functions
	AttributePath(
		const std::function<T_out(const T_node*)>& func_get,
		const std::function<bool(T_node*, const T_out& val)> func_set,
		std::optional<T_out> default_value = {})
		: func_get(func_get),
		  func_set(func_set),
		  default_value(default_value)
	{}

	/*
	* Receive the value
	*/
	T_out get_value(const T_node* node) const
	{
		try
		{
			return func_get(node);
		}
		catch (const std::exception& e)
		{
			if (default_value.has_value()) return default_value.value();

			// If no default value shall be returned: throw anyway
			throw e; // A AttributePath-Getter Function throwed and no default value was set
		}
		
	}

	bool set_value(T_node* node, const T_out& val)
	{
		return func_set(node, val);
	}

	/*
	* True if the value exists
	*/
	bool has_value(const T_node* node) const
	{
		try {
			func_get(node);
		}
		catch (const std::exception&) { return false; }

		return true;
	}
};

/*
* FilterAttributePath allows to compare AttributePath to certain values
*/
template <typename T_node>
class FilterBase
{
public:
	virtual bool is_within(const T_node* e) const = 0;
};

/*
* This class wraps a function which may return a boolean
* indicating if the passed node is accepted
* 
* This wrapper is present so one can mix FilterFunctions and AttributePath&Values
*/
template<typename T_node>
class FilterFunction : public FilterBase<T_node>
{
	
public:
	std::function<bool(const T_node*)> func;

	FilterFunction(std::function<bool(const T_node*)> func)
		: func(func)
	{}

	/*
	* returns true if the stored function would return true getting the passed parameters
	*/
	bool is_within(const T_node* node) const override
	{
		return func(node);
	}

	// convenience functions
	/*
	* \brief Get a FilterFunctions which always returns true
	*/
	static FilterFunction<T_node>* accept_all()
	{
		return new FilterFunction<T_node>([](const T_node* node) { return true; });
	}

	/*
	* \brief Get a FilterFunctions which always returns false
	*/
	static FilterFunction<T_node>* accept_none()
	{
		return new FilterFunction<T_node>([](const T_node* node) { return false; });
	}
};

/*
* This class wraps values and a corresponding path to these values.
* It allows to check if a node has one of the stored values in the given AttributePath
* 
* This could've been implemented deriving from AttributPathFilterFunction by creating lambda functions
* but doing so would make swapping allowed_vals more exhaustive
*/
template<typename T_node, typename T_out>
class FilterAttributePath : public FilterBase<T_node>
{
public:
	std::vector<T_out> allowed_vals = {};
	AttributePath<T_node, T_out> path;

	// Functions
	FilterAttributePath(const AttributePath<T_node, T_out>& path, const std::vector<T_out>& allowed_vals = {})
		: path(path),
		  allowed_vals(allowed_vals)
	{}

	/*
	* Returns true if the passed node contains an attribute which is present in allowed_vals
	*/
	bool is_within(const T_node* node) const override
	{
		// this may throw a logic error due to AttributePath::get_value
		const T_out val = path.get_value(node);

		for (const T_out comp_val : allowed_vals)
		{
			if (val == comp_val) return true;
		}
		return false;
	}
};

/*
* Select all DiagramElememts matching a set of filters
*/
template<typename T_node>
class SelectiveView
{
public:
	std::vector<T_node*> view_nodes;
	std::function<const std::vector<T_node*> (const T_node*)> func_get_children;
	

	/*
	* \brief like other constructor. Accepts a a function returning a const view of child nodes
	*/
	SelectiveView(std::function<const std::vector<T_node*>(const T_node*)> func_get_children, const T_node* root = nullptr, std::vector<FilterBase<T_node>*> filters = {})
		:func_get_children(func_get_children)
	{
		if (root != nullptr) apply_filter(root, filters);
	}
	
	// Functions
	/*
	* \brief resets current view and selects elements for which all filters return true
	*/
	void apply_filter(const T_node* root, std::vector<FilterBase<T_node>*> filters)
	{
		view_nodes.clear();

		//grab all node pointers
		std::vector<T_node*> nodes = {};

		//dump all childs from a parent
		std::function<void(const T_node*)> node_traverser = [&](const T_node* parent)
		{
			const std::vector<T_node*> _nodes = func_get_children(parent);
			for (T_node* child : _nodes)
			{
				// register
				nodes.push_back(child);
				// traverse children
				node_traverser(child);
			}
		};

		node_traverser(root);

		// apply filter objects on each node
		for (T_node* node : nodes)
		{
			bool flag = true;

			for(const FilterBase<T_node>* filter : filters)
			{
				if (!filter->is_within(node))
				{
					flag = false;
					break;
				}
			}

			// add if all filters were passed
			if(flag) view_nodes.push_back(node);
		}
	}
};

/*
* Contains some kind of bijective transition from T_in to T_out.
*/
template<typename T_node, typename T_in, typename T_out>
class BijeciveModifier
{
public:
	std::function<T_out(T_in)> func_apply;
	std::function<T_in(T_out)> func_revert;

	BijeciveModifier(std::function<T_out(T_in)> func_apply, std::function<T_in(T_out)> func_revert)
		: func_apply(func_apply),
		  func_revert(func_revert)
	{}

	T_out apply(T_in value) const
	{
		return func_apply(value);
	}

	T_in revert(T_out value) const
	{
		return func_revert(value);
	}

	/*
	* Validate the stored functions.
	* True if the fuctions are bijective for the given values
	*/
	bool is_valid(const AttributePath<T_node, T_in>& path, const SelectiveView<T_node>& values) const
	{
		// check for bijectivity
		std::unordered_map<T_in, T_out> mapping_apply;
		std::unordered_map<T_out, T_in> mapping_reverse;

		for(const T_node* node : values.view_nodes)
		{
			const T_in val = path.get_value(node);
			const T_out val_modified = apply(val);
			const T_in val_reversed = revert(val_modified);

			// check if the functions are surjective for {value_x} -> {value_y}
			if (val != val_reversed) return false;
			
			// if this returns: apply mapping is not injective
			auto output = mapping_apply.find(val);
			if (output != mapping_apply.end() && output->second != val_modified) return false;
			else mapping_apply.insert({ val, val_modified });

			// if this returns: revert is not injective
			auto input = mapping_reverse.find(val_modified);
			if (input != mapping_reverse.end() && input->second != val) return false;
			else mapping_reverse.insert({ val_modified, val });
		}

		return true;
	}

	/*
	* Generate a dummy modifier for type transformation.
	  Same function for transforming and reverting
	*/
	BijeciveModifier<T_node, T_in, T_out> static passthrough(const std::function<T_out(T_in)>& func_passthrough)
	{
		return BijeciveModifier<T_node, T_in, T_out>(func_passthrough, func_passthrough);
	}


	/*
	* \brief Chain this BijectiveModifier with another one and return the newly created Modifier
	*/
	template<typename T_out_new>
	BijeciveModifier<T_node, T_in, T_out_new> chain_with(BijeciveModifier<T_node, T_out, T_out_new>& modifier_second)
	{
		return BijeciveModifier<T_node, T_in, T_out_new>(
			[modifier_first = *this, modifier_second](const T_in& val)->T_out_new
			{
				T_out cache = modifier_first.apply(val);
				return modifier_second.apply(cache);
			},
			[modifier_first = *this, modifier_second](const T_out_new& val)->T_in
			{
				T_out cache_reverse = modifier_second.revert(val);
				return modifier_first.revert(cache_reverse);
			}
			);
	}
};

/*
* This class represents a column
* All in all it just stores a list of values which may be displayed as a column
*/
template <typename T_node>
class ColumnBase
{
public:

	virtual std::vector<std::string> build(const SelectiveView<T_node>& values) const = 0;
	virtual std::string header() const = 0;

	virtual bool is_valid(const SelectiveView<T_node>& values) const { return false; }
	virtual bool sync_with(const SelectiveView<T_node>& values, std::vector<std::string> col_out) { return false; }
};

template<typename T_node, typename T_val>
class Column : public ColumnBase<T_node>
{
public:
	AttributePath<T_node, T_val> linked_attr;
	BijeciveModifier<T_node, T_val, std::string> modifier_pipe;
	std::string _header;

	/*
	* \brief Construct a column
	* \param linked_attr: v alu to exctract from a node
	* \param modifier_pipe: modifier resulting into a string output of the linked attribute
	* \param header: name of the column
	*/
	Column(const AttributePath<T_node, T_val>& linked_attr, BijeciveModifier<T_node, T_val, std::string> modifier_pipe, std::string header)
		: linked_attr(linked_attr),
		modifier_pipe(modifier_pipe),
		_header(header)
	{}

	/*
	* \brief Iterate over all values and apply the stored Modifier on them
	*/
	std::vector<std::string> build(const SelectiveView<T_node>& values) const
	{
		std::vector<std::string> ret_vector{ header() };
		for (const T_node* node : values.view_nodes)
		{
			const T_val val = linked_attr.get_value(node);
			const std::string val_modified = modifier_pipe.apply(val);

			ret_vector.push_back(val_modified);
		}

		return ret_vector;
	}

	/*
	* \brief  Check if the combination of values, AttributePath and BijectiveModifier is valid
	*/
	bool is_valid(const SelectiveView<T_node>& values) const override
	{
		for (const T_node* node : values.view_nodes)
		{
			// predict get_value behaviour. Return false if it is going to throw
			if (!linked_attr.has_value(node) && !linked_attr.default_value.has_value()) return false;
		}

		return modifier_pipe.is_valid(linked_attr, values);
	}

	/*
	* 
	*/
	bool sync_with(const SelectiveView<T_node>& values, std::vector<std::string> col_out) override
	{
		// Check if modifier is valid for all values in the view
		if (!is_valid(values)) return false;

		// -1 to remove header
		if (values.view_nodes.size() != col_out.size()-1) return false;

		// get transformed set
		std::vector<std::string> col_build = build(values);
		// drop header
		col_build.erase(col_build.begin());

		// get original set
		std::vector<T_val> col_originals = {};
		for (const std::string& val : col_build)
		{
			col_originals.push_back(modifier_pipe.revert(val));
		}

		// drop header
		
		col_out.erase(col_out.begin());

		// store T_vals which are new due to the sync
		std::vector<T_val> reached_vals = {};
		std::vector<std::string> reached_val_keys = {};

		// Check if there are collisions with the currently existing sets
		for(const std::string& val : col_out)
		{
			if (std::find(col_build.begin(), col_build.end(), val) == col_build.end())
			{
				//check if pipe is bijective for this value
				const T_val reverted = modifier_pipe.revert(val);
				const std::string applied = modifier_pipe.apply(reverted);

				// same value after transition?
				if (applied != val) return false;

				// reverted value already in set?
				if (std::find(col_originals.begin(), col_originals.end(), reverted) != col_originals.end()) return false;

				//cache reverted val to check it later
				reached_vals.push_back(reverted);
				reached_val_keys.push_back(applied);
			}
		}
		// check if there is a collision in reached vals
		for(size_t i = 0; i < reached_vals.size(); i++)
		{
			size_t count_val = std::count(reached_vals.begin(), reached_vals.end(), reached_vals.at(i));
			size_t count_key = std::count(reached_val_keys.begin(), reached_val_keys.end(), reached_val_keys.at(i));

			if (count_val != count_key) return false;
		}

		bool success_flag = true;

		// acutally apply changes
		for(size_t i = 0; i < col_out.size(); i++)
		{
			T_node* node = values.view_nodes.at(i);
			T_val val_new = modifier_pipe.revert(col_out.at(i));
			T_val val_old = linked_attr.get_value(node);

			// set value
			if (val_old != val_new) 
			{
				success_flag &= linked_attr.set_value(node, val_new);
				std::cout << "comparing " << val_old << " " << val_new << " new flag state: " << success_flag << "\n";
			}
		}
		return success_flag;
	}

	std::string header() const override
	{
		return _header;
	}
};

template <typename T_node>
class BijectiveAlgorithm
{
public:
	SelectiveView<T_node> view;
	std::vector<ColumnBase<T_node>*> columns;

	

	BijectiveAlgorithm(const SelectiveView<T_node>& view) 
		: view(view)
	{}

	/*
	* \brief generate the datatable according to current members
	*/
	std::vector<std::vector<std::string>> apply()
	{
		std::vector<std::vector<std::string>> table;

		for(ColumnBase<T_node>* col : columns)
		{
			const std::vector<std::string> res = col->build(view);
			table.push_back(res);
		}

		return table;
	}

	void register_column(ColumnBase<T_node>* col)
	{
		columns.push_back(col);
	}

	bool is_valid()
	{
		for(ColumnBase<T_node>* col : columns)
		{
			if (!col->is_valid(view)) return false;
		}
		return true;
	}

	/*
	* \brief try to sync the passed tree via the vector. Resolving is done by vector position!
	*        if this function returns false some modifiers were not set
	*/
	bool sync_with(T_node* tree, std::vector<std::vector<std::string>> table)
	{
		// This object must be valid
		if (!is_valid()) return false;

		// mapping is done by vector position
		if (table.size() != columns.size()) return false;

		// check if size of each col is similar to the present amount nodes
		const size_t view_size = view.view_nodes.size();
		
		// -1 to skip header
		for(const std::vector<std::string>& col_output : table) if (col_output.size()-1 != view_size) return false;

		// start syncing col for col
		bool success_flag = true;

		for(size_t i = 0; i < table.size(); i++)
		{
			// header is removed in columns sync_with function
			bool ret_val = columns.at(i)->sync_with(view, table.at(i));
			success_flag &= ret_val;
			std::cout << "col finishsed .. success flag is " << success_flag << "| ret val: " << ret_val << '\n';
		}
		return success_flag;
	}

	/*
	* Helper Functions - mainly constructor forwarders with some template types set
	*/

	/*
	* \brief Initialize a column-object. Returns a pointer
	*/
	template<typename T_path_input>
	Column<T_node, T_path_input>* make_column(
		const AttributePath<T_node, T_path_input>& linked_attr,
		const BijeciveModifier<T_node, T_path_input, std::string>& modifier_pipe,
		std::string col_name = "colname not set")
	{
		return new Column<T_node, T_path_input>(linked_attr, modifier_pipe, col_name);
	}

	/*
	* \brief generate an AttribtuePath-object
	*/
	template<typename T_out>
	AttributePath<T_node, T_out> make_path(
		std::function<T_out(const T_node*)> get_function,
		std::function<bool(T_node*, const T_out&)> set_function,
		std::optional<T_out> default_value = {})
	{
		return AttributePath<T_node, T_out>(get_function, set_function, default_value);
	}

	/*
	* \brief generate a BijectiveModifer-object
	*/
	template<typename T_in, typename T_out>
	BijeciveModifier<T_node, T_in, T_out> make_modifier(std::function<T_out(T_in)> func_apply, std::function<T_in(T_out)> func_revert)
	{
		return BijeciveModifier<T_node, T_in, T_out>(func_apply, func_revert);
	}
};