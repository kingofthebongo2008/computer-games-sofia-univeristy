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

		struct animals
		{
			lip::reloc_array < animal >          m_animals;

			animals()
			{
			}

			explicit animals(const lip::load_context& c) : m_animals(c)
			{

			}

			LIP_DECLARE_RTTI()
		};

		LIP_DECLARE_TYPE_ID(uc::lip::animal)
		LIP_DECLARE_TYPE_ID(uc::lip::animals)
		LIP_DECLARE_TYPE_ID(uc::lip::reloc_array < animal >)

		LIP_BEGIN_DEFINE_RTTI(animal)
			LIP_RTTI_MEMBER(animal, m_legs)
			LIP_RTTI_MEMBER(animal, m_ears)
		LIP_END_DEFINE_RTTI(animal)

		LIP_BEGIN_DEFINE_RTTI(animals)
			LIP_RTTI_MEMBER(animals, m_animals)
		LIP_END_DEFINE_RTTI(animals)
	}
}

std::vector<uint8_t> package_animals()
{
	using namespace uc::lip;

	animals as;

	as.m_animals.push_back({ 1, 2 });
	as.m_animals.push_back({ 3, 3 });

	
	for (auto&& a : as.m_animals)
	{
		std::wcout << a.m_ears << L" " << a.m_legs << "\n";
	}

	return binarize_object(&as);
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

		for (auto&& a : bs->m_animals)
		{
			std::wcout << a.m_ears << L" " << a.m_legs << "\n";
		}
	}

	return 0;
}
