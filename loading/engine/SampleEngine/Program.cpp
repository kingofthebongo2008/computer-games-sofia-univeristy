#include "pch.h"
#include <iostream>
#include <fstream>


#include <uc/lip/lip.h>
#include <uc/lip/tools_time_utils.h>


using namespace winrt;

namespace uc
{
	namespace lip
	{
		struct animal
		{
			uint32_t m_legs;
			uint32_t m_ears;

			LIP_DECLARE_RTTI()
		};

		struct fish
		{
			uint32_t	 m_eyes;
			LIP_DECLARE_RTTI()
		};

		struct animals
		{
			lip::reloc_array < animal >          m_animals;
			lip::reloc_pointer< fish  >			 m_fish;

			animals()
			{

			}

			explicit animals(const lip::load_context& c) : m_animals(c), m_fish(c)
			{

			}

			LIP_DECLARE_RTTI()
		};

		LIP_DECLARE_TYPE_ID(uc::lip::fish)
		LIP_DECLARE_TYPE_ID(uc::lip::animal)
		LIP_DECLARE_TYPE_ID(uc::lip::animals)
		LIP_DECLARE_TYPE_ID(uc::lip::reloc_array < animal >)
		LIP_DECLARE_TYPE_ID(uc::lip::reloc_pointer< fish > )

			

		LIP_BEGIN_DEFINE_RTTI(animal)
			LIP_RTTI_MEMBER(animal, m_legs)
			LIP_RTTI_MEMBER(animal, m_ears)
		LIP_END_DEFINE_RTTI(animal)

		LIP_BEGIN_DEFINE_RTTI(animals)
			LIP_RTTI_MEMBER(animals, m_animals)
			LIP_RTTI_MEMBER(animals, m_fish)
		LIP_END_DEFINE_RTTI(animals)

		LIP_BEGIN_DEFINE_RTTI(fish)
			LIP_RTTI_MEMBER(fish, m_eyes)
		LIP_END_DEFINE_RTTI(fish)
	}
}

namespace
{
	using namespace uc::lip;

	void print_animal(const animals* as)
	{
		for (auto&& a : as->m_animals)
		{
			std::wcout << "Ears:" << a.m_ears  << "\n";
			std::wcout << "Legs:" << a.m_legs << "\n";
		}

		std::wcout << "Fish eyes:" << as->m_fish->m_eyes << "\n";
	}

	std::vector<uint8_t> package_animals()
	{
		animals as;

		as.m_animals.push_back({ 1, 2 });
		as.m_animals.push_back({ 3, 3 });
		as.m_fish = make_unique<fish>();
		as.m_fish->m_eyes = 5;

		print_animal(&as);

		return binarize_object(&as);
	}
}


int main()
{
	std::wcout << "Packaging Animals..." << "\n";
	std::vector<uint8_t> blob = package_animals();

	std::wcout << "Restoring.." << "\n";

	{
		using namespace uc::lip;
		load_context ctx = make_load_context(&blob[0]);
		animals* bs = placement_new<animals>(ctx);

		print_animal(bs);

		bs->~animals();
	}

	return 0;
}
