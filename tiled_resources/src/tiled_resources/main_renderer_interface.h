#pragma once

#include <Unknwn.h>
#include "window_environment.h"

//Simple renderer system interface.
//Made to separate the windowing system from the code renderer itself
class IMainRenderer
{
	public:

    virtual ~IMainRenderer() = default;

    virtual void Initialize() = 0;
    virtual void Uninitialize() = 0;
    virtual void Load() = 0;
    virtual void Run() = 0 ;

    virtual void OnWindowSizeChanged(const sample::window_environment& envrionment) = 0;
    virtual void SetWindow(IUnknown* w, const sample::window_environment& envrionment) = 0;
};


IMainRenderer* MakeMainRenderer();

