#include "pch.h"
#include "App.h"

#include <ppltasks.h>

using namespace lab_1;

using namespace concurrency;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

// Функция main используется только для инициализации класса IFrameworkView.
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	auto direct3DApplicationSource = ref new Direct3DApplicationSource();
	CoreApplication::Run(direct3DApplicationSource);
	return 0;
}

IFrameworkView^ Direct3DApplicationSource::CreateView()
{
	return ref new App();
}

App::App() :
	m_windowClosed(false),
	m_windowVisible(true)
{
}

// Первый метод, вызванный при создании IFrameworkView.
void App::Initialize(CoreApplicationView^ applicationView)
{
	// Зарегистрируйте обработчики событий для жизненного цикла приложения. Этот пример включает событие Activated, чтобы
	// можно было сделать объект CoreWindow активным и запустить прорисовку в окне.
	applicationView->Activated +=
		ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);

	CoreApplication::Suspending +=
		ref new EventHandler<SuspendingEventArgs^>(this, &App::OnSuspending);

	CoreApplication::Resuming +=
		ref new EventHandler<Platform::Object^>(this, &App::OnResuming);

	// На этом этапе у нас есть доступ к устройству. 
	// Мы можем создавать ресурсы, зависящие от устройства.
	m_deviceResources = std::make_shared<DX::DeviceResources>();
}

// Вызывается при создании (или повторном создании) объекта CoreWindow.
void App::SetWindow(CoreWindow^ window)
{
	window->SizeChanged += 
		ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &App::OnWindowSizeChanged);

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &App::OnVisibilityChanged);

	window->Closed += 
		ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &App::OnWindowClosed);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDpiChanged);

	currentDisplayInformation->OrientationChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnOrientationChanged);

	DisplayInformation::DisplayContentsInvalidated +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDisplayContentsInvalidated);

	m_deviceResources->SetWindow(window);
}

// Инициализирует ресурсы сцены или загружает ранее сохраненное состояние приложения.
void App::Load(Platform::String^ entryPoint)
{
	if (m_main == nullptr)
	{
		m_main = std::unique_ptr<lab_1Main>(new lab_1Main(m_deviceResources));
	}
}

// Этот метод вызывается после того, как окно становится активным.
void App::Run()
{
	while (!m_windowClosed)
	{
		if (m_windowVisible)
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

			m_main->Update();

			if (m_main->Render())
			{
				m_deviceResources->Present();
			}
		}
		else
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
}

// Требуется для IFrameworkView.
// События Terminate не приводят к вызову метода Uninitialize. Этот метод вызывается, если класс IFrameworkView
// уничтожается, пока приложение находится на переднем плане.
void App::Uninitialize()
{
}

// Обработчики событий жизненного цикла приложения.

void App::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	// Run() не запускается, пока не будет активирован объект CoreWindow.
	CoreWindow::GetForCurrentThread()->Activate();
}

void App::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	// Асинхронное сохранение состояния приложения после запроса задержки. Удержание задержки
	// означает, что приложение занято выполнением приостановки операций. Обратите внимание,
	// что задержку невозможно удерживать в течение долгого времени. Примерно через пять
	// секунд произойдет принудительный выход из приложения.
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

	create_task([this, deferral]()
	{
        m_deviceResources->Trim();

		// Вставьте сюда свой код.

		deferral->Complete();
	});
}

void App::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Восстановление всех данных и состояний, которые были выгружены при приостановке действия. По умолчанию данные
	// и состояние сохраняются при возобновлении работы. Обратите внимание, что этого
	// не происходит, если работа приложения была остановлена ранее.

	// Вставьте сюда свой код.
}

// Обработчики событий окна.

void App::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	m_deviceResources->SetLogicalSize(Size(sender->Bounds.Width, sender->Bounds.Height));
	m_main->CreateWindowSizeDependentResources();
}

void App::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
}

void App::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;
}

// Обработчики события DisplayInformation

void App::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
	// Примечание. Значение LogicalDpi, полученное здесь, может не соответствовать фактическому DPI приложения,
	// если его масштаб изменяется для устройств с экраном высокого разрешения. После установки DPI в DeviceResources,
	// всегда следует получать его с помощью метода GetDpi.
	// См. DeviceResources.cpp для получения дополнительных сведений.
	m_deviceResources->SetDpi(sender->LogicalDpi);
	m_main->CreateWindowSizeDependentResources();
}

void App::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	m_deviceResources->SetCurrentOrientation(sender->CurrentOrientation);
	m_main->CreateWindowSizeDependentResources();
}

void App::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	m_deviceResources->ValidateDevice();
}