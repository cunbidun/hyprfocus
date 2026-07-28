#include "Log.hpp"
#include <hyprland/src/devices/IPointer.hpp>
#include <hyprland/src/helpers/WLClasses.hpp>
#include <regex>

#include "Globals.hpp"
#include <hyprland/src/config/values/types/BoolValue.hpp>
#include <hyprland/src/config/values/types/StringValue.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/helpers/signal/Signal.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include <hyprland/src/debug/log/Logger.hpp>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/animation/AnimationManager.hpp>

#include "Flash.hpp"
#include "Shrink.hpp"

PHLWINDOW g_pPreviouslyFocusedWindow = nullptr;
bool g_bMouseWasPressed = false;
bool g_bWorkspaceChanged = false;

std::unordered_map<std::string, std::unique_ptr<IFocusAnimation>> g_mAnimations;

static CHyprSignalListener g_lActiveWindow;
static CHyprSignalListener g_lMouseButton;

static bool OnSameWorkspace(PHLWINDOW pWindow1, PHLWINDOW pWindow2) {
  if (pWindow1 == pWindow2) {
    return true;
  } else if (pWindow1 == nullptr || pWindow2 == nullptr) {
    return false;
  } else if (pWindow1->workspaceID() == pWindow2->workspaceID()) {
    return true;
  } else {
    return false;
  }
}

void flashWindow(PHLWINDOW pWindow) {
  static const auto focusAnimation =
      CConfigValue<Config::STRING>("plugin:hyprfocus:focus_animation");
  hyprfocus_log(Log::INFO, "Flashing window");
  hyprfocus_log(Log::INFO, "Animation: {}", *focusAnimation);

  auto it = g_mAnimations.find(*focusAnimation);
  if (it != g_mAnimations.end()) {
    hyprfocus_log(Log::INFO, "Calling onWindowFocus for animation {}",
                  *focusAnimation);
    it->second->onWindowFocus(pWindow, PHANDLE);
  }
}

SDispatchResult flashCurrentWindow(std::string) {
  hyprfocus_log(Log::INFO, "Flashing current window");
  SDispatchResult result = {
      .passEvent = false,
      .success = true,
  };
  static const auto PHYPRFOCUSENABLED =
      CConfigValue<Config::BOOL>("plugin:hyprfocus:enabled");
  if (!*PHYPRFOCUSENABLED) {
    const std::string message = "HyprFocus is disabled";
    hyprfocus_log(Log::INFO, "HyprFocus is disabled");
    result.success = false;
    result.error = message;
    return result;
  }
  if (g_pPreviouslyFocusedWindow == nullptr) {
    const std::string message = "No previously focused window";
    hyprfocus_log(Log::INFO, "No previously focused window");
    result.success = false;
    result.error = message;
    return result;
  }
  result.passEvent = true;
  flashWindow(g_pPreviouslyFocusedWindow);
  return result;
}

APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
  PHANDLE = handle;

  HyprlandAPI::addConfigValueV2(PHANDLE,
                                makeShared<Config::Values::CBoolValue>(
                                    "plugin:hyprfocus:enabled", "", false));
  HyprlandAPI::addConfigValueV2(
      PHANDLE, makeShared<Config::Values::CBoolValue>(
                   "plugin:hyprfocus:animate_workspacechange", "", true));
  HyprlandAPI::addConfigValueV2(
      PHANDLE, makeShared<Config::Values::CBoolValue>(
                   "plugin:hyprfocus:animate_floating", "", true));
  HyprlandAPI::addConfigValueV2(
      PHANDLE, makeShared<Config::Values::CStringValue>(
                   "plugin:hyprfocus:focus_animation", "", "flash"));
  HyprlandAPI::addConfigValueV2(PHANDLE,
                                makeShared<Config::Values::CStringValue>(
                                    "plugin:hyprfocus:exclude_class", "", ""));
  HyprlandAPI::addDispatcherV2(PHANDLE, "animatefocused", &flashCurrentWindow);

  g_mAnimations["flash"] = std::make_unique<CFlash>();
  g_mAnimations["shrink"] = std::make_unique<CShrink>();
  g_mAnimations["none"] = std::make_unique<IFocusAnimation>();

  for (auto &[name, pAnimation] : g_mAnimations) {
    pAnimation->init(PHANDLE, name);
    hyprfocus_log(Log::INFO, "Registered animation: {}", name);
  }

  HyprlandAPI::reloadConfig();
  Animation::mgr()->tick();
  hyprfocus_log(Log::INFO, "Reloaded config");

  g_lActiveWindow = Event::bus()->m_events.window.active.listen(
      [](const PHLWINDOW &pWindow, Desktop::eFocusReason) {
        try {
          hyprfocus_log(Log::INFO, "Active window changed");

          static const auto PHYPRFOCUSENABLED =
              CConfigValue<Config::BOOL>("plugin:hyprfocus:enabled");
          static const auto PANIMATEFLOATING =
              CConfigValue<Config::BOOL>("plugin:hyprfocus:animate_floating");
          static const auto PANIMATEWORKSPACECHANGE =
              CConfigValue<Config::BOOL>(
                  "plugin:hyprfocus:animate_workspacechange");

          if (!*PHYPRFOCUSENABLED) {
            hyprfocus_log(Log::INFO, "HyprFocus is disabled");
            return;
          }
          if (pWindow == nullptr) {
            hyprfocus_log(Log::INFO, "Window is null");
            return;
          }
          if (pWindow == g_pPreviouslyFocusedWindow) {
            hyprfocus_log(Log::INFO, "Window is the same as the previous one");
            return;
          }
          if (pWindow->m_isFloating && !*PANIMATEFLOATING) {
            hyprfocus_log(Log::INFO, "Floating window, not animating");
            g_pPreviouslyFocusedWindow = pWindow;
            return;
          }
          if (!*PANIMATEWORKSPACECHANGE &&
              !OnSameWorkspace(pWindow, g_pPreviouslyFocusedWindow)) {
            hyprfocus_log(Log::INFO, "Workspace changed, not animating");
            g_pPreviouslyFocusedWindow = pWindow;
            return;
          }

          static const auto PEXCLUDECLASS =
              CConfigValue<Config::STRING>("plugin:hyprfocus:exclude_class");
          const std::string excludeClass(*PEXCLUDECLASS);
          if (!excludeClass.empty()) {
            try {
              if (std::regex_search(pWindow->m_class,
                                    std::regex(excludeClass))) {
                hyprfocus_log(Log::INFO,
                              "Window class {} matches exclude_class, skipping",
                              pWindow->m_class);
                g_pPreviouslyFocusedWindow = pWindow;
                return;
              }
            } catch (const std::regex_error &re) {
              hyprfocus_log(Log::ERR, "Invalid exclude_class regex: {}",
                            re.what());
            }
          }

          flashWindow(pWindow);
          g_pPreviouslyFocusedWindow = pWindow;

        } catch (std::exception &e) {
          hyprfocus_log(Log::ERR, "Error: {}", e.what());
        }
      });
  hyprfocus_log(Log::INFO, "Registered active window change callback");

  g_lMouseButton = Event::bus()->m_events.input.mouse.button.listen(
      [](IPointer::SButtonEvent event, Event::SCallbackInfo &) {
        try {
          hyprfocus_log(Log::INFO, "Mouse button state: {}", (int)event.state);
          g_bMouseWasPressed = (int)event.state == 1;
        } catch (std::exception &e) {
          hyprfocus_log(Log::ERR, "Error: {}", e.what());
        }
      });
  hyprfocus_log(Log::INFO, "Registered mouse button callback");

  HyprlandAPI::reloadConfig();

  return {"hyprfocus", "animate windows on focus", "Vortex", "2.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
  g_lActiveWindow.reset();
  g_lMouseButton.reset();
}
