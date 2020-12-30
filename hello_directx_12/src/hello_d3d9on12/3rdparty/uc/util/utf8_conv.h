//////////////////////////////////////////////////////////////////////////
//
// FILE: utf8conv.h
//
// Header file defining prototypes of helper functions for converting 
// strings between Unicode UTF-8 and UTF-16.
// (The implementation file is "utf8conv_inl.h").
//
// UTF-8 text is stored in std::string; 
// UTF-16 text is stored in std::wstring.
//
// This code just uses Win32 Platform SDK and C++ standard library; 
// so it can be used also with the Express editions of Visual Studio.
//
//
// Original code: February 4th, 2011
// Last update:   October 15th, 2011
//
// - Added more information to the utf8_conversion_error class
//   (like the return code of ::GetLastError());
//   moreover, the class now derives from std::runtime_error.
//
// - Added conversion function overloads taking raw C strings as input.
//   (This is more efficient when there are raw C strings already
//   available, because it avoids the creation of temporary
//   new std::[w]string's.)
//
// - UTF-8 conversion functions now detect invalid UTF-8 sequences
//   thanks to MB_ERR_INVALID_CHARS flag, and throw an exception
//   in this case.
//
//
// by Giovanni Dicanio <gdicanio@mvps.org>
//
//////////////////////////////////////////////////////////////////////////

#ifndef __UTILITY_UTF8_CONV_H__
#define __UTILITY_UTF8_CONV_H__


//------------------------------------------------------------------------
//                              INCLUDES
//------------------------------------------------------------------------

#include <cstdint>
#include <stdexcept>    // std::runtime_error
#include <string>       // STL string classes

#include <windows.h>

namespace uc
{
    namespace util
    {

        //------------------------------------------------------------------------
        // Exception class representing an error occurred during UTF-8 conversion.
        //------------------------------------------------------------------------
        class utf8_conversion_error : public std::runtime_error
        {
        public:

            //
            // Naming convention note:
            // -----------------------
            //
            // This exception class is derived from std::runtime_error class,
            // so I chose to use the same naming convention of STL classes
            // (e.g. do_something_intersting() instead of DoSomethingInteresting()).
            //


            // Error code type 
            // (a DWORD, as the return value type from ::GetLastError())
            typedef unsigned long error_code_type;

            // Type of conversion
            enum conversion_type
            {
                conversion_utf8_from_utf16,     // UTF-16 ---> UTF-8
                conversion_utf16_from_utf8      // UTF-8  ---> UTF-16
            };


            // Constructs an UTF-8 conversion error exception 
            // with a raw C string message, conversion type and error code.
            utf8_conversion_error(const char * message, conversion_type conversion, error_code_type error_code);


            // Constructs an UTF-8 conversion error exception 
            // with a std::string message, conversion type and error code.
            utf8_conversion_error(const std::string & message, conversion_type conversion, error_code_type error_code);


            // Returns the type of conversion (UTF-8 from UTF-16, or vice versa)
            conversion_type conversion() const;


            // Returns the error code occurred during the conversion
            // (which is typically the return value of ::GetLastError()).
            error_code_type error_code() const;

            //
            // IMPLEMENTATION
            //

        private:
            conversion_type m_conversion;   // kind of conversion
            error_code_type m_error_code;   // error code
        };

        //------------------------------------------------------------------------
        // Converts a string from UTF-8 to UTF-16.
        // On error, can throw an utf8_conversion_error exception.
        //------------------------------------------------------------------------
        std::wstring utf16_from_utf8(const std::string & utf8);

        //------------------------------------------------------------------------
        // Converts a raw C string from UTF-8 to UTF-16.
        // On error, can throw an utf8_conversion_error exception.
        // If the input pointer is NULL, an empty string is returned.
        //------------------------------------------------------------------------
        std::wstring utf16_from_utf8(const char * utf8);

        //------------------------------------------------------------------------
        // Converts a string from UTF-16 to UTF-8.
        // On error, can throw an utf8_conversion_error exception.
        //------------------------------------------------------------------------
        std::string utf8_from_utf16(const std::wstring & utf16);


        //------------------------------------------------------------------------
        // Converts a raw C string from UTF-16 to UTF-8.
        // On error, can throw an utf8_conversion_error exception.
        // If the input pointer is NULL, an empty string is returned.
        //------------------------------------------------------------------------
        std::string utf8_from_utf16(const wchar_t * utf16);

        //------------------------------------------------------------------------
        //      Implementation of utf8_conversion_error class methods
        //------------------------------------------------------------------------

        inline utf8_conversion_error::utf8_conversion_error(const char * message, conversion_type conversion, error_code_type error_code) :
            std::runtime_error(message)
            , m_conversion(conversion)
            , m_error_code(error_code)
        {

        }

        inline utf8_conversion_error::utf8_conversion_error(const std::string & message, conversion_type conversion, error_code_type error_code) :
            std::runtime_error(message)
            , m_conversion(conversion)
            , m_error_code(error_code)
        {

        }

        inline utf8_conversion_error::conversion_type utf8_conversion_error::conversion() const
        {
            return m_conversion;
        }


        inline utf8_conversion_error::error_code_type utf8_conversion_error::error_code() const
        {
            return m_error_code;
        }

        //------------------------------------------------------------------------
        //              Implementation of module functions
        //------------------------------------------------------------------------

        inline std::wstring utf16_from_utf8(const std::string & utf8)
        {
            //
            // Special case of empty input string
            //
            if (utf8.empty())
            {
                return std::wstring();
            }

            // Fail if an invalid input character is encountered
            const DWORD conversionFlags = MB_ERR_INVALID_CHARS;


            //
            // Get length (in wchar_t's) of resulting UTF-16 string
            //
            const int utf16Length = ::MultiByteToWideChar
            (
                CP_UTF8,            // convert from UTF-8
                conversionFlags,    // flags
                utf8.data(),        // source UTF-8 string
                (int32_t)utf8.length(),      // length (in chars) of source UTF-8 string
                NULL,               // unused - no conversion done in this step
                0                   // request size of destination buffer, in wchar_t's
            );

            if (utf16Length == 0)
            {
                // Error
                DWORD error = ::GetLastError();

                throw utf8_conversion_error
                (
                    (error == ERROR_NO_UNICODE_TRANSLATION) ?
                    "Invalid UTF-8 sequence found in input string." :
                    "Can't get length of UTF-16 string (MultiByteToWideChar failed).",
                    utf8_conversion_error::conversion_utf16_from_utf8,
                    error
                );
            }


            //
            // Allocate destination buffer for UTF-16 string
            //
            std::wstring utf16;
            utf16.resize(utf16Length);


            //
            // Do the conversion from UTF-8 to UTF-16
            //
            if (!::MultiByteToWideChar
            (
                CP_UTF8,            // convert from UTF-8
                0,                  // validation was done in previous call, 
                                    // so speed up things with default flags
                utf8.data(),        // source UTF-8 string
                (int32_t)utf8.length(),      // length (in chars) of source UTF-8 string
                &utf16[0],          // destination buffer
                (int32_t)utf16.length()      // size of destination buffer, in wchar_t's
            )
                )
            {
                // Error
                DWORD error = ::GetLastError();
                throw utf8_conversion_error
                (
                    "Can't convert string from UTF-8 to UTF-16 (MultiByteToWideChar failed).",
                    utf8_conversion_error::conversion_utf16_from_utf8,
                    error
                );
            }

            //
            // Return resulting UTF-16 string
            //
            return utf16;
        }

        inline std::wstring utf16_from_utf8(const char * utf8)
        {
            //
            // Special case of empty input string
            //
            if (utf8 == NULL || *utf8 == '\0')
            {
                return std::wstring();
            }


            // Prefetch the length of the input UTF-8 string
            const int utf8Length = static_cast<int>(strlen(utf8));

            // Fail if an invalid input character is encountered
            const DWORD conversionFlags = MB_ERR_INVALID_CHARS;

            //
            // Get length (in wchar_t's) of resulting UTF-16 string
            //
            const int utf16Length = ::MultiByteToWideChar
            (
                CP_UTF8,            // convert from UTF-8
                conversionFlags,    // flags
                utf8,               // source UTF-8 string
                utf8Length,         // length (in chars) of source UTF-8 string
                NULL,               // unused - no conversion done in this step
                0                   // request size of destination buffer, in wchar_t's
            );

            if (utf16Length == 0)
            {
                // Error
                DWORD error = ::GetLastError();
                throw utf8_conversion_error(
                    (error == ERROR_NO_UNICODE_TRANSLATION) ?
                    "Invalid UTF-8 sequence found in input string." :
                    "Can't get length of UTF-16 string (MultiByteToWideChar failed).",
                    utf8_conversion_error::conversion_utf16_from_utf8,
                    error);
            }


            //
            // Allocate destination buffer for UTF-16 string
            //
            std::wstring utf16;
            utf16.resize(utf16Length);


            //
            // Do the conversion from UTF-8 to UTF-16
            //
            if (!::MultiByteToWideChar
            (
                CP_UTF8,            // convert from UTF-8
                0,                  // validation was done in previous call, 
                                    // so speed up things with default flags
                utf8,               // source UTF-8 string
                utf8Length,         // length (in chars) of source UTF-8 string
                &utf16[0],          // destination buffer
                (int32_t)utf16.length()      // size of destination buffer, in wchar_t's
            )
                )
            {
                // Error
                DWORD error = ::GetLastError();
                throw utf8_conversion_error(
                    "Can't convert string from UTF-8 to UTF-16 (MultiByteToWideChar failed).",
                    utf8_conversion_error::conversion_utf16_from_utf8,
                    error);
            }


            //
            // Return resulting UTF-16 string
            //
            return utf16;
        }



        inline std::string utf8_from_utf16(const std::wstring & utf16)
        {
            //
            // Special case of empty input string
            //
            if (utf16.empty())
            {
                return std::string();
            }


            //
            // Get length (in chars) of resulting UTF-8 string
            //
            const int utf8Length = ::WideCharToMultiByte(
                CP_UTF8,            // convert to UTF-8
                0,                  // default flags
                utf16.data(),       // source UTF-16 string
                (int32_t)utf16.length(),     // source string length, in wchar_t's,
                NULL,               // unused - no conversion required in this step
                0,                  // request buffer size
                NULL, NULL          // unused
            );

            if (utf8Length == 0)
            {
                // Error
                DWORD error = ::GetLastError();
                throw utf8_conversion_error(
                    "Can't get length of UTF-8 string (WideCharToMultiByte failed).",
                    utf8_conversion_error::conversion_utf8_from_utf16,
                    error);
            }


            //
            // Allocate destination buffer for UTF-8 string
            //
            std::string utf8;
            utf8.resize(utf8Length);


            //
            // Do the conversion from UTF-16 to UTF-8
            //
            if (!::WideCharToMultiByte(
                CP_UTF8,                // convert to UTF-8
                0,                      // default flags
                utf16.data(),           // source UTF-16 string
                (int32_t)utf16.length(),         // source string length, in wchar_t's,
                &utf8[0],               // destination buffer
                (int32_t)utf8.length(),          // destination buffer size, in chars
                NULL, NULL              // unused
            ))
            {
                // Error
                DWORD error = ::GetLastError();
                throw utf8_conversion_error(
                    "Can't convert string from UTF-16 to UTF-8 (WideCharToMultiByte failed).",
                    utf8_conversion_error::conversion_utf8_from_utf16,
                    error);
            }


            //
            // Return resulting UTF-8 string
            //
            return utf8;
        }

        inline std::string utf8_from_utf16(const wchar_t * utf16)
        {
            //
            // Special case of empty input string
            //
            if (utf16 == NULL || *utf16 == L'\0')
                return std::string();


            // Prefetch the length of the input UTF-16 string
            const int utf16Length = static_cast<int>(wcslen(utf16));


            //
            // Get length (in chars) of resulting UTF-8 string
            //
            const int utf8Length = ::WideCharToMultiByte(
                CP_UTF8,            // convert to UTF-8
                0,                  // default flags
                utf16,              // source UTF-16 string
                utf16Length,        // source string length, in wchar_t's,
                NULL,               // unused - no conversion required in this step
                0,                  // request buffer size
                NULL, NULL          // unused
            );
            if (utf8Length == 0)
            {
                // Error
                DWORD error = ::GetLastError();
                throw utf8_conversion_error(
                    "Can't get length of UTF-8 string (WideCharToMultiByte failed).",
                    utf8_conversion_error::conversion_utf8_from_utf16,
                    error);
            }


            //
            // Allocate destination buffer for UTF-8 string
            //
            std::string utf8;
            utf8.resize(utf8Length);


            //
            // Do the conversion from UTF-16 to UTF-8
            //
            if (!::WideCharToMultiByte(
                CP_UTF8,                // convert to UTF-8
                0,                      // default flags
                utf16,                  // source UTF-16 string
                utf16Length,            // source string length, in wchar_t's,
                &utf8[0],               // destination buffer
                (int32_t)utf8.length(),          // destination buffer size, in chars
                NULL, NULL              // unused
            ))
            {
                // Error
                DWORD error = ::GetLastError();
                throw utf8_conversion_error(
                    "Can't convert string from UTF-16 to UTF-8 (WideCharToMultiByte failed).",
                    utf8_conversion_error::conversion_utf8_from_utf16,
                    error);
            }


            //
            // Return resulting UTF-8 string
            //
            return utf8;
        }

    } // namespace utf8util
}


//////////////////////////////////////////////////////////////////////////




#endif

