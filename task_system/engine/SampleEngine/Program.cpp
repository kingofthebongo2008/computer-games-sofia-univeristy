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

namespace renderer
{
	class component;

	class component_handle
	{
		component* m_pointer;
	};

	struct render_object_allocator
	{
		void* make_object( type )
		{

		}

		void free_object(type t)
		{

		}
	};

	struct visibility_object_allocator
	{
		void* make_object( type )
		{

		}

		void free_object( type t )
		{

		}
	};
}

namespace game
{
	class object
	{
		public:

		virtual std::vector< renderer::component_handle > make_components( render_object_allocator* r, visibility_object_allocator* r) = 0;
	};
}

namespace visibility
{
	class object;
}

namespace renderer
{
	class object;

	class component
	{
		object*					m_render_object;
		visibility::object*		m_visibility_object;
	};

	class object
	{
		component*				m_render_component;
	};

	class world
	{
		std::vector<object*>	m_objects;
	};
}

namespace visibility
{
	class object
	{
		renderer::object*	m_render_object;
	};

	class world
	{
		std::vector< object* > m_objects;
	};
}

class mechanic	final : public game::object
{
	renderer::component_handle m_render_components[1];
};

class room final : public game::object
{
	renderer::component_handle m_render_components[17];
};


int main()
{

}
