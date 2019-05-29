#include "pch.h"
#include <iostream>
#include <fstream>

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
		HANDLE				 m_complete_event;

		CompletionList()
		{
			throw_if_failed(CreateUmsCompletionList(&m_list));
			throw_if_failed(GetUmsCompletionListEvent(m_list, &m_complete_event));
		}

		~CompletionList()
		{
			CloseHandle(m_complete_event);
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

	struct SchedulerInfo
	{
		SchedulerInfo()
		{
			m_workerThreadEvents[0] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			m_workerThreadEvents[1] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		}

		~SchedulerInfo()
		{
			CloseHandle(m_workerThreadEvents[0]);
			CloseHandle(m_workerThreadEvents[1]);
		}

		HANDLE			m_workerThreadEvents[2];	//two worker threads
		CompletionList	m_completionList;			//list with tasks per scheduler
	};

	enum ActivationPayload
	{
		BlockedOnTrap = 0,
		BlockedOnSystemCall = 1
	};
}

DWORD WINAPI UMSWorkerThread(_In_ LPVOID lpParameter)
{
	lpParameter;
	UmsThreadYield(0);
	std::wcout<<L"Work1";

	while (true)
	{
		std::wcout << L"Handle1";
		Sleep(50);
	}
	return 0;
}

thread_local Ums::CompletionList* s_list;	

void WINAPI UmsScheduler(UMS_SCHEDULER_REASON Reason, ULONG_PTR ActivationPayload, PVOID SchedulerParam)
{
	Ums::CompletionList* list = nullptr;

	switch (Reason)
	{
		case UmsSchedulerStartup:
										std::wcout << L"List" << '\n';
										list = reinterpret_cast<Ums::CompletionList*>(SchedulerParam);
										s_list = list;
										break;
		case UmsSchedulerThreadBlocked:
		case UmsSchedulerThreadYield:
										list = s_list;
										std::wcout << L"List2" << '\n';;
										break;

		default:
										break;
	}

	std::wcout << L"List3" << '\n';;
	HANDLE handles[1]	= { list->m_complete_event};
	DWORD handle_index	= WaitForMultipleObjects(1, handles, FALSE, INFINITE);

	if (handle_index == 0)
	{
		std::wcout << L"Handle" << '\n';;
	}

	std::wcout << L"List4";
}

DWORD WINAPI UMSSchedulerThread(_In_ LPVOID lpParameter)
{
	lpParameter;

	UMS_SCHEDULER_STARTUP_INFO s = {};
	Ums::SchedulerInfo*  info = reinterpret_cast<Ums::SchedulerInfo*>(lpParameter);
	Ums::CompletionList* list = &info->m_completionList;
	
	s.UmsVersion			  = UMS_VERSION;
	s.SchedulerProc			  = UmsScheduler;
	s.CompletionList	      = list->m_list;
	s.SchedulerParam		  = list;

	if ( EnterUmsSchedulingMode(&s) )
	{
		//block until worker threads exit
		WaitForMultipleObjects(2, &info->m_workerThreadEvents[0], TRUE, INFINITE);
		return 0;
	}
	else
	{
		return 1;
	}
}

int main()
{
	//Kernel threads
	HANDLE						umsSchedulerPool[2] = {};
	DWORD						umsSchedulerPoolId[2] = {};
	Ums::SchedulerInfo			umsBlockinginfos[2];

	//UMS threads
	HANDLE						umsWorkdrPool[4] = {};
	DWORD						umsWorkerPoolId[4] = {};
	Ums::Context				umsThreadContexts[4];
	Ums::AttributeList			umsAttributes[4];

	umsAttributes[0].Update(&umsThreadContexts[0], &umsBlockinginfos[0].m_completionList);
	umsAttributes[1].Update(&umsThreadContexts[1], &umsBlockinginfos[0].m_completionList);

	umsAttributes[2].Update(&umsThreadContexts[2], &umsBlockinginfos[1].m_completionList);
	umsAttributes[3].Update(&umsThreadContexts[3], &umsBlockinginfos[1].m_completionList);

	SIZE_T stackSize = 0;

	umsSchedulerPool[0]			= CreateThread(nullptr, stackSize, &UMSSchedulerThread, &umsBlockinginfos[0], 0, &umsSchedulerPoolId[0]);
	umsSchedulerPool[1]			= CreateThread(nullptr, stackSize, &UMSSchedulerThread, &umsBlockinginfos[1], 0, &umsSchedulerPoolId[1]);

	Sleep(1000);
	auto err = GetLastError();

	umsWorkdrPool[0]			= CreateRemoteThreadEx(GetCurrentProcess(), nullptr, stackSize, &UMSWorkerThread, nullptr, 0, umsAttributes[0].m_ctx, &umsWorkerPoolId[0]);
	auto err1 = GetLastError();

	umsWorkdrPool[1]			= CreateRemoteThreadEx(GetCurrentProcess(), nullptr, stackSize, &UMSWorkerThread, nullptr, 0, umsAttributes[1].m_ctx, &umsWorkerPoolId[1]);
	umsWorkdrPool[2]			= CreateRemoteThreadEx(GetCurrentProcess(), nullptr, stackSize, &UMSWorkerThread, nullptr, 0, umsAttributes[2].m_ctx, &umsWorkerPoolId[2]);
	umsWorkdrPool[3]			= CreateRemoteThreadEx(GetCurrentProcess(), nullptr, stackSize, &UMSWorkerThread, nullptr, 0, umsAttributes[3].m_ctx, &umsWorkerPoolId[3]);

	auto err2 = GetLastError();

	Sleep(10000000);
    return 0;
}
