#include "pch.h"
#include "file_helper.h"
#include "error.h"


namespace sample
{
	// Function that reads from a binary file asynchronously.
	inline Concurrency::task<std::vector<uint8_t>> ReadDataAsync(const std::wstring& filename)
	{
		using namespace Windows::Storage;
		using namespace Concurrency;

		auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;

		return create_task(folder->GetFileAsync(Platform::StringReference(filename.c_str()))).then([](StorageFile ^ file)
		{
			return FileIO::ReadBufferAsync(file);
		}).then([](Streams::IBuffer ^ fileBuffer) -> std::vector<uint8_t>
		{
			std::vector<uint8_t> returnBuffer;
			returnBuffer.resize(fileBuffer->Length);
			Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(Platform::ArrayReference<uint8_t>(returnBuffer.data(), fileBuffer->Length));
			return returnBuffer;
		});
	}
}