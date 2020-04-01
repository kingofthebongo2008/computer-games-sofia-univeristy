#pragma once

#if defined(_X86) || defined(_X64) || defined(_ARM) || defined(_ARM64)
	#define UC_MATH_CALL __vectorcall
#else
	#define UC_MATH_CALL __fastcall
#endif





