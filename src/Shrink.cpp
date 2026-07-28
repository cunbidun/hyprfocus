#include "Shrink.hpp"
#include "Log.hpp"

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/layout/LayoutManager.hpp>
#include <hyprland/src/animation/AnimationManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

void CShrink::init(HANDLE pHandle, std::string animationName) {
  IFocusAnimation::init(pHandle, "shrink");
  addFloatConfigValue(pHandle, "shrink_percentage", 0.95f);
}

void CShrink::setup(HANDLE pHandle, std::string animationName) {}

void CShrink::onWindowFocus(PHLWINDOW pWindow, HANDLE pHandle) {
  IFocusAnimation::onWindowFocus(pWindow, pHandle);

  static const auto shrinkPercentage =
      CConfigValue<Config::FLOAT>(configPrefix() + "shrink_percentage");

  const float scale = std::clamp(*shrinkPercentage, 0.1f, 1.0f);

  hyprfocus_log(Log::INFO, "Shrink: percentage={}", scale);

  auto &realPosition = pWindow->positionAnimation();
  auto &realSize = pWindow->sizeAnimation();

  const Vector2D ORIGINAL_POS = realPosition->goal();
  const Vector2D ORIGINAL_SIZE = realSize->goal();

  const Vector2D newSize = ORIGINAL_SIZE * scale;
  const Vector2D newPos = ORIGINAL_POS + (ORIGINAL_SIZE - newSize) / 2.f;

  realPosition->setConfig(m_sFocusOutAnimConfig);
  realSize->setConfig(m_sFocusOutAnimConfig);

  *realPosition = newPos;
  *realSize = newSize;

  realSize->setCallbackOnEnd(
      [w = PHLWINDOWREF{pWindow}, this, ORIGINAL_POS,
       ORIGINAL_SIZE](WP<CBaseAnimatedVariable> pav) {
        if (!w)
          return;

        auto &realPosition = w->positionAnimation();
        auto &realSize = w->sizeAnimation();

        realSize->setConfig(m_sFocusInAnimConfig);
        realPosition->setConfig(m_sFocusInAnimConfig);

        if (w->m_isFloating) {
          *realPosition = ORIGINAL_POS;
          *realSize = ORIGINAL_SIZE;
        } else {
          g_layoutManager->recalculateMonitor(w->m_monitor.lock());
        }

        realSize->setCallbackOnEnd(nullptr);
      });
}
