#include "IFocusAnimation.hpp"
#include "Globals.hpp"

#include <hyprland/src/config/shared/animation/AnimationTree.hpp>
#include <hyprland/src/config/values/types/FloatValue.hpp>
#include <hyprland/src/config/values/types/StringValue.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprlang.hpp>

void IFocusAnimation::init(HANDLE pHandle, std::string animationName) {
  m_szAnimationName = animationName;
  hyprfocus_log(Log::INFO, "Initializing focus animation: {}", animationName);

  addStringConfigValue(pHandle, "in_bezier", "default");
  addStringConfigValue(pHandle, "out_bezier", "default");
  addFloatConfigValue(pHandle, "in_speed", 1.f);
  addFloatConfigValue(pHandle, "out_speed", 5.f);

  m_sFocusInAnimConfig =
      Config::animationTree()->getAnimationPropertyConfig("global");
  m_sFocusInAnimConfig->internalEnabled = 1;
  m_sFocusInAnimConfig->internalStyle =
      std::string("hyprfocus_") + animationName + std::string("_in");
  m_sFocusInAnimConfig->pValues = m_sFocusInAnimConfig;

  m_sFocusOutAnimConfig =
      Config::animationTree()->getAnimationPropertyConfig("global");
  m_sFocusOutAnimConfig->internalEnabled = 1;
  m_sFocusOutAnimConfig->internalStyle =
      std::string("hyprfocus_") + animationName + std::string("_out");
  m_sFocusOutAnimConfig->pValues =
      CWeakPointer<SAnimationPropertyConfig>(m_sFocusOutAnimConfig);
}

void IFocusAnimation::setup(HANDLE pHandle, std::string animationName) {
  // hyprfocus_log(Log::INFO, "Setting up focus animation: {}", animationName);
}

void IFocusAnimation::onWindowFocus(PHLWINDOW pWindow, HANDLE pHandle) {
  hyprfocus_log(Log::INFO, "Base callback for animation: {}",
                m_szAnimationName);
  static const auto inBezier =
      CConfigValue<Config::STRING>(configPrefix() + "in_bezier");
  static const auto inSpeed =
      CConfigValue<Config::FLOAT>(configPrefix() + "in_speed");
  static const auto outBezier =
      CConfigValue<Config::STRING>(configPrefix() + "out_bezier");
  static const auto outSpeed =
      CConfigValue<Config::FLOAT>(configPrefix() + "out_speed");
  m_sFocusInAnimConfig->internalBezier = *inBezier;
  m_sFocusInAnimConfig->internalSpeed = *inSpeed;

  m_sFocusOutAnimConfig->internalBezier = *outBezier;
  m_sFocusOutAnimConfig->internalSpeed = *outSpeed;

  hyprfocus_log(
      Log::INFO, "In bezier: {} In speed: {} Out bezier: {} Out speed: {}",
      m_sFocusInAnimConfig->internalBezier, m_sFocusInAnimConfig->internalSpeed,
      m_sFocusOutAnimConfig->internalBezier,
      m_sFocusOutAnimConfig->internalSpeed);
}

void IFocusAnimation::addFloatConfigValue(HANDLE pHandle, std::string name,
                                          Config::FLOAT value) {
  m_vConfigNames.push_back(configPrefix() + name);
  HyprlandAPI::addConfigValueV2(pHandle,
                                makeShared<Config::Values::CFloatValue>(
                                    m_vConfigNames.back().c_str(), "", value));
  hyprfocus_log(Log::INFO, "Added config value: {}", configPrefix() + name);
}

void IFocusAnimation::addStringConfigValue(HANDLE pHandle, std::string name,
                                           Config::STRING value) {
  m_vConfigNames.push_back(configPrefix() + name);
  HyprlandAPI::addConfigValueV2(pHandle,
                                makeShared<Config::Values::CStringValue>(
                                    m_vConfigNames.back().c_str(), "", value));
  hyprfocus_log(Log::INFO, "Added config value: {}", configPrefix() + name);
}
