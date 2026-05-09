#pragma once

#include <hyprlang.hpp>
#define WLR_USE_UNSTABLE

#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprutils/animation/AnimationConfig.hpp>

#include <deque>

using namespace Hyprutils::Memory;
using namespace Hyprutils::Animation;

class IFocusAnimation {
public:
  virtual void onWindowFocus(PHLWINDOW pWindow, HANDLE pHandle);
  virtual void init(HANDLE pHandle, std::string animationName);
  virtual void setup(HANDLE pHandle, std::string animationName);

  void addFloatConfigValue(HANDLE pHandle, std::string name,
                           Config::FLOAT value);
  void addStringConfigValue(HANDLE pHandle, std::string name,
                            Config::STRING value);

public:
  CSharedPointer<SAnimationPropertyConfig> m_sFocusInAnimConfig =
      makeShared<SAnimationPropertyConfig>();
  CSharedPointer<SAnimationPropertyConfig> m_sFocusOutAnimConfig =
      makeShared<SAnimationPropertyConfig>();

  std::string m_szAnimationName;
  std::deque<std::string> m_vConfigNames;

  std::string configPrefix() {
    return std::string("plugin:hyprfocus:") + m_szAnimationName + ":";
  }
};
