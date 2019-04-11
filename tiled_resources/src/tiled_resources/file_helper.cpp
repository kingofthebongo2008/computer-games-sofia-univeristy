#include "pch.h"
#include "file_helper.h"
#include "error.h"
#include <future>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>

#include <experimental/resumable>

namespace sample
{
	using namespace winrt;
	using namespace winrt::Windows::Foundation;
	using namespace winrt::Windows::Foundation::Collections;
	using namespace winrt::Windows::Storage;

	using namespace std::experimental;
	using namespace Concurrency;

	// Function that reads from a binary file asynchronously.
	IAsyncOperation< Streams::IBuffer > ReadDataAsync(const std::wstring& filename)
	{
		using namespace Concurrency;

		auto folder = winrt::Windows::ApplicationModel::Package::Current().InstalledLocation();

		auto file		 = co_await folder.GetFileAsync(filename.c_str());
		auto r			 = co_await FileIO::ReadBufferAsync(file);
		return r;
	}

	concurrency::task< std::vector<uint8_t> > ReadFileAsync(const std::wstring& filename)
	{
		auto buffer = co_await ReadDataAsync(filename);
		auto length = buffer.Length();

		std::vector <uint8_t > v;
		v.resize(length);
		winrt::array_view<uint8_t> view(v);
		Streams::DataReader::FromBuffer(buffer).ReadBytes(view);
		co_return v;
	}
	
}