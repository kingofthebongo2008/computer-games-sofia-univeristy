#include "pch.h"
#include <iostream>
#include <fstream>

using namespace winrt;

struct task
{
	virtual void execute() = delete;
	virtual ~task() = delete;
};

void throw_if_failed(BOOL v)
{
	if (!v)
	{
		DWORD e = GetLastError();
		__debugbreak();
	}
}

namespace Ums
{
	struct CompletionList
	{
		PUMS_COMPLETION_LIST m_list = {};

		CompletionList()
		{
			throw_if_failed(CreateUmsCompletionList(&m_list));
		}

		~CompletionList()
		{
			DeleteUmsCompletionList(m_list);
		}
	};

	struct Context
	{
		PUMS_CONTEXT m_ctx = {};

		Context()
		{
			throw_if_failed(CreateUmsThreadContext(&m_ctx));
		}

		~Context()
		{
			DeleteUmsThreadContext(m_ctx);
		}
	};

	struct AttributeList
	{
		std::vector<uint8_t>		 m_attribute_list_memory;
		LPPROC_THREAD_ATTRIBUTE_LIST m_ctx = {};
		SIZE_T						 m_ctxSize = 0;

		AttributeList()
		{
			InitializeProcThreadAttributeList(nullptr, 1, 0, &m_ctxSize);
			SetLastError(0);
			m_attribute_list_memory.resize(m_ctxSize);
			m_ctx = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(&m_attribute_list_memory[0]);
			throw_if_failed(InitializeProcThreadAttributeList(m_ctx, 1, 0, &m_ctxSize));
		}

		void Update( Context* c, CompletionList* l )
		{
			UMS_CREATE_THREAD_ATTRIBUTES s = {};
			s.UmsVersion = UMS_VERSION;
			s.UmsCompletionList = l->m_list;
			s.UmsContext = c->m_ctx;
			throw_if_failed(UpdateProcThreadAttribute(m_ctx, 0, PROC_THREAD_ATTRIBUTE_UMS_THREAD, &s, sizeof(s), nullptr, nullptr));
		}

		~AttributeList()
		{
			DeleteProcThreadAttributeList(m_ctx);
		}
	};
}

DWORD WINAPI UMSWorkerThread(_In_ LPVOID lpParameter)
{
	lpParameter;
	return 0;
}

void WINAPI UmsScheduler(UMS_SCHEDULER_REASON Reason, ULONG_PTR ActivationPayload, PVOID SchedulerParam)
{
}

DWORD WINAPI UMSSchedulerThread(_In_ LPVOID lpParameter)
{
	lpParameter;

	UMS_SCHEDULER_STARTUP_INFO s = {};
	Ums::CompletionList* list = reinterpret_cast<Ums::CompletionList*>(lpParameter);

	s.UmsVersion = UMS_VERSION;
	s.SchedulerProc = UmsScheduler;
	s.CompletionList = list->m_list;

	if ( EnterUmsSchedulingMode(&s) )
	{
		Sleep(10000);
		return 0;
	}
	else
	{
		return -1;
	}
}

int main()
{
	//Kernel threads
	HANDLE						umsSchedulerPool[2];
	DWORD						umsSchedulerPoolId[2];
	Ums::CompletionList			umscompletionList[2];

	//UMS threads
	HANDLE						umsWorkdrPool[4];
	DWORD						umsWorkerPoolId[4];
	Ums::Context				umsThreadContexts[4];
	Ums::AttributeList			umsAttributes[4];

	umsAttributes[0].Update(&umsThreadContexts[0], &umscompletionList[0]);
	umsAttributes[1].Update(&umsThreadContexts[1], &umscompletionList[0]);

	umsAttributes[2].Update(&umsThreadContexts[2], &umscompletionList[1]);
	umsAttributes[3].Update(&umsThreadContexts[3], &umscompletionList[1]);

	umsSchedulerPool[0]			= CreateThread(nullptr, 0, &UMSSchedulerThread, &umscompletionList[0], 0, &umsSchedulerPoolId[0]);
	umsSchedulerPool[1]			= CreateThread(nullptr, 0, &UMSSchedulerThread, &umscompletionList[1], 0, &umsSchedulerPoolId[1]);

	umsWorkdrPool[0]			= CreateRemoteThreadEx(GetCurrentProcess(), nullptr, 0, &UMSWorkerThread, 0, 0, umsAttributes[0].m_ctx, &umsWorkerPoolId[0]);
	umsWorkdrPool[1]			= CreateRemoteThreadEx(GetCurrentProcess(), nullptr, 0, &UMSWorkerThread, 0, 0, umsAttributes[1].m_ctx, &umsWorkerPoolId[1]);
	umsWorkdrPool[2]			= CreateRemoteThreadEx(GetCurrentProcess(), nullptr, 0, &UMSWorkerThread, 0, 0, umsAttributes[2].m_ctx, &umsWorkerPoolId[2]);
	umsWorkdrPool[3]			= CreateRemoteThreadEx(GetCurrentProcess(), nullptr, 0, &UMSWorkerThread, 0, 0, umsAttributes[3].m_ctx, &umsWorkerPoolId[3]);

	Sleep(10000);
    return 0;
}
