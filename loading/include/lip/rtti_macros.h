#pragma once

#define LIP_ENABLE_RTTI

#ifdef LIP_ENABLE_RTTI

	#ifndef LIP_DECLARE_RTTI
		#define LIP_DECLARE_RTTI()\
		class registrar\
		{\
			public:\
			registrar();\
		};\
		static registrar g_registrar;
	#endif


	#ifndef LIP_BEGIN_DEFINE_RTTI
		#define LIP_BEGIN_DEFINE_RTTI(ClassName)\
		ClassName::registrar::registrar()\
		{\
		    static ::uc::lip::introspector<ClassName> g_introspector;\
            static ::uc::lip::type_factory_registrar<ClassName> g_typed_factory;
	#endif

	#ifndef LIP_END_DEFINE_RTTI
		#define LIP_END_DEFINE_RTTI(ClassName)\
        ::uc::lip::introspector_database()->register_introspector( lip::make_type_info<ClassName>().type_id(), &g_introspector );\
        g_introspector.sort();\
		}\
		ClassName::registrar	ClassName::g_registrar;
	#endif

	#ifndef LIP_RTTI_BASE_CLASS
		#define LIP_RTTI_BASE_CLASS(ClassName)\
		g_introspector.register_base_class<ClassName>();
	#endif

	#ifndef LIP_RTTI_MEMBER
		#define LIP_RTTI_MEMBER(ClassName,MemberName)\
		g_introspector.register_member(#MemberName,&ClassName::MemberName);
	#endif

#else

	#ifndef LIP_DECLARE_RTTI
		#define LIP_DECLARE_RTTI()
	#endif

	#ifndef LIP_BEGIN_DEFINE_RTTI
		#define LIP_BEGIN_DEFINE_RTTI(ClassName)
	#endif

	#ifndef LIP_END_DEFINE_RTTI
		#define LIP_END_DEFINE_RTTI(ClassName)
	#endif

	#ifndef LIP_RTTI_BASE_CLASS
		#define LIP_RTTI_BASE_CLASS(ClassName)
	#endif

	#ifndef LIP_RTTI_MEMBER
		#define LIP_RTTI_MEMBER(ClassName,MemberName)
	#endif

#endif



