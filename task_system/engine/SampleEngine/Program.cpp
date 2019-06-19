#include "pch.h"
#include <iostream>
#include <fstream>

using namespace winrt;

namespace math
{

}

struct quaternion
{
	float m_components[4];
};

struct world_transform
{
	float m_rotation[4];

	float m_translation_x;
	float m_translation_y;
	float m_translation_z;
	float m_scale;
};

class renderer_component;

class renderer_component_handle
{
	renderer_component* m_pointer;
};

struct render_object_allocator
{
	enum type
	{
		mechanic,
		room,
		count
	};

	template <typename o >
	o* make_object( type )
	{
		return nullptr;
	}

	template <typename o>
	void free_object(type t, o* o )
	{

	}
};

struct visibility_object_allocator
{
	enum type
	{

	};

	template <typename o >
	o* make_object(type)
	{
		return nullptr;
	}

	template <typename o>
	void free_object(type t, o* o)
	{
		
	}
};

struct scratch_pad_allocator
{
	void* allocate(size_t s) { return nullptr; };
};

void* operator new (std::size_t size, scratch_pad_allocator* ptr) throw()
{
	return ptr->allocate(size);
}

void* f()
{
	scratch_pad_allocator* p;
	return new (p) float;
}

struct scratch_pad_deleter
{
	void operator()(void*) const
	{

	}
};

template <typename t> std::unique_ptr<t, scratch_pad_deleter > make_unique( scratch_pad_allocator* p )
{
	return unique_ptr<t, scratch_pad_deleter >(new (p) t());
}

struct gpu_resources_allocator
{
	void* create_texture_2d() { return nullptr; };
	void* create_buffer() { return nullptr; };
	void* create_geometry() { return nullptr; };
};

struct gpu_command_list
{

};

struct make_components_allocators
{
	render_object_allocator* m_roa;
	visibility_object_allocator* m_voa;
	scratch_pad_allocator* m_soa;
	gpu_resources_allocator* m_ga;
	gpu_command_list* m_cmd_list;
};

class game_object
{
	public:

	std::vector< renderer_component_handle >		 make_components(make_components_allocators* allocators) { return on_make_components(allocators); }

	protected:

	virtual ~game_object() {};
	
	private:

	virtual std::vector< renderer_component_handle > on_make_components(make_components_allocators* allocators) = 0;
};

class visibility_object;
class renderer_object;

class renderer_component
{
	renderer_object*				m_render;
	visibility_object*				m_visibility;
};

class renderer_object
{
	public:
	game_object*					m_game_object;
};

class renderer_world
{
	public:
	std::vector<renderer_object*>	m_objects;
};

class visibility_object
{
	renderer_object*	m_render;
};

class visibility_world
{
	public:
	std::vector< visibility_object* > m_objects;
};

class mechanic	final : public game_object
{
	private:

	class mechanic_render_object final : public renderer_object
	{

	};

	std::vector< renderer_component_handle > on_make_components(make_components_allocators* allocators) override
	{
		std::vector< renderer_component_handle > res;
		return res;
	}

	renderer_component_handle m_render_components[1];
};

class room final : public game_object
{
	private:

	std::vector< renderer_component_handle > on_make_components(make_components_allocators* allocators) override
	{
		std::vector< renderer_component_handle > res;
		return res;
	}

	renderer_component_handle m_render_components[17];
};

int main()
{
	make_components_allocators  alloc;

	renderer_world				rw;
	visibility_world			vw;

	room						room;
	mechanic					mechanic;
	
	{
		for (auto&& c : room.make_components(&alloc))
		{
			
		}
	}
}
