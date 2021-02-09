#include "pch.h"
#include "lab_1Main.h"
#include "Common\DirectXHelper.h"

using namespace lab_1;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;

// Загружает и инициализирует ресурсы приложения во время его загрузки.
lab_1Main::lab_1Main(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources)
{
	// Регистрация для получения уведомлений о том, что устройство потеряно или создано заново
	m_deviceResources->RegisterDeviceNotify(this);

	// TODO: замените это инициализацией содержимого своего приложения.
	m_sceneRenderer = std::unique_ptr<Sample3DSceneRenderer>(new Sample3DSceneRenderer(m_deviceResources));

	m_fpsTextRenderer = std::unique_ptr<SampleFpsTextRenderer>(new SampleFpsTextRenderer(m_deviceResources));

	// TODO: измените настройки таймера, если требуется режим, отличный от режима по умолчанию с переменным шагом по времени.
	// например, для логики обновления с фиксированным временным шагом 60 кадров/с вызовите:
	/*
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);
	*/
}

lab_1Main::~lab_1Main()
{
	// Отмена регистрации уведомлений устройства
	m_deviceResources->RegisterDeviceNotify(nullptr);
}

// Обновляет состояние приложения при изменении размера окна (например, при изменении ориентации устройства)
void lab_1Main::CreateWindowSizeDependentResources() 
{
	// TODO: замените это инициализацией содержимого вашего приложения в зависимости от размера.
	m_sceneRenderer->CreateWindowSizeDependentResources();
}

// Обновляет состояние приложения один раз за кадр.
void lab_1Main::Update() 
{
	// Обновление объектов сцены.
	m_timer.Tick([&]()
	{
		// TODO: замените это функциями обновления содержимого своего приложения.
		m_sceneRenderer->Update(m_timer);
		m_fpsTextRenderer->Update(m_timer);
	});
}

// Прорисовывает текущий кадр в соответствии с текущим состоянием приложения.
// Возвращает значение true, если кадр прорисован и готов к отображению.
bool lab_1Main::Render() 
{
	// Не пытаться выполнять какую-либо прорисовку до первого обновления.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Выполните сброс окна просмотра для нацеливания на весь экран.
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	// Сброс целевых объектов прорисовки в экран.
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	// Очистка заднего буфера и представления трафарета глубины.
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Прорисовка объектов сцены.
	// TODO: замените это функциями прорисовки содержимого своего приложения.
	m_sceneRenderer->Render();
	m_fpsTextRenderer->Render();

	return true;
}

// Уведомляет визуализаторы о том, что ресурсы устройства необходимо освободить.
void lab_1Main::OnDeviceLost()
{
	m_sceneRenderer->ReleaseDeviceDependentResources();
	m_fpsTextRenderer->ReleaseDeviceDependentResources();
}

// Уведомляет визуализаторы о том, что ресурсы устройства можно создать заново.
void lab_1Main::OnDeviceRestored()
{
	m_sceneRenderer->CreateDeviceDependentResources();
	m_fpsTextRenderer->CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
