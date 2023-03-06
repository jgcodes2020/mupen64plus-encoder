#ifndef _M64PP_CORE_HPP_
#define _M64PP_CORE_HPP_

#define ENC_SUPPORT
#include <mupen64plus/m64p_common.h>
#include <mupen64plus/m64p_encoder.h>
#include <mupen64plus/m64p_frontend.h>
#include <mupen64plus/m64p_plugin.h>
#include <mupen64plus/m64p_vcr.h>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../oslib/plibdl.hpp"
#include "../oslib/strconv.hpp"
#include "fnptr.hpp"
#include "mupen64plus/m64p_types.h"
#include "ntuple.hpp"

// NTFN: named tuple function
#define M64P_NTFN(name) nt_leaf<#name, ptr_##name>
#define M64P_LDFN(handle, name) oslib::pdlsym<ptr_##name>((handle), #name)
namespace m64p {

  class core {
  public:
    core(const char* path) :
      lib_handle(oslib::pdlopen(path)), fn_table(init_fn_table(lib_handle)) {
      // Startup core
      {
        m64p_error err = get_fn<"CoreStartup">()(
          0x020000, nullptr, nullptr, this,
          [](void* self, int level, const char* msg) {
            reinterpret_cast<core*>(self)->debug_log(
              static_cast<m64p_msg_level>(level), msg
            );
          },
          this,
          [](void* self, m64p_core_param param, int value) {
            reinterpret_cast<core*>(self)->run_state_handlers(param, value);
          }
        );

        if (err != M64ERR_SUCCESS) {
          throw std::runtime_error(get_fn<"CoreErrorMessage">()(err));
        }

        get_fn<"VCR_SetErrorCallback">()(vcr_debug_log);
      }
    }

    ~core() {
      get_fn<"CoreShutdown">()();
      oslib::pdlclose(lib_handle);
    }

    static int vcr_debug_log(m64p_msg_level level, const char* msg) {
      return 0;
      switch (level) {
        case M64MSG_ERROR:
          std::cout << "[VCR ERROR] " << msg << '\n';
          break;
        case M64MSG_WARNING:
          std::cout << "[VCR WARN]  " << msg << '\n';
          break;
        case M64MSG_INFO:
          std::cout << "[VCR INFO]  " << msg << '\n';
          break;
        case M64MSG_STATUS:
          std::cout << "[VCR STAT]  " << msg << '\n';
          break;
        case M64MSG_VERBOSE:
          // std::cout << "[VCR TRACE] " << msg << '\n';
          break;
      }
      return 0;
    }
    // Log a message from Mupen64Plus to the console.
    void debug_log(m64p_msg_level level, const char* msg) {
      return;
      switch (level) {
        case M64MSG_ERROR:
          std::cout << "[M64+ ERROR] " << msg << '\n';
          break;
        case M64MSG_WARNING:
          std::cout << "[M64+ WARN]  " << msg << '\n';
          break;
        case M64MSG_INFO:
          std::cout << "[M64+ INFO]  " << msg << '\n';
          break;
        case M64MSG_STATUS:
          std::cout << "[M64+ STAT]  " << msg << '\n';
          break;
        case M64MSG_VERBOSE:
          // std::cout << "[M64+ TRACE] " << msg << '\n';
          break;
      }
    }

    // Adds a state handler.
    void add_state_handler(
      fnptr<bool(m64p_core_param, int)> fn, bool before = false
    ) {
      if (before)
        handlers.push_front(fn);
      else
        handlers.push_back(fn);
    }

    // Overrides the video extension functions.
    // Will error unless all functions are overridden.
    void override_vidext(m64p_video_extension_functions& fns) {
      m64p_check(get_fn<"CoreOverrideVidExt">()(&fns));
    }

    // Opens a ROM file from a path.
    void open_rom(const char* path) {
      auto str_path = oslib::utf8_to_path(path);
      {
        std::ifstream input(str_path.c_str());
        input.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        // get size
        input.seekg(0, std::ios_base::end);
        size_t buffer_size = input.tellg();
        input.seekg(0, std::ios_base::beg);

        auto buffer = std::make_unique<char[]>(buffer_size);
        input.read(buffer.get(), buffer_size);

        debug_log(
          M64MSG_INFO,
          static_cast<std::ostringstream&&>(
            std::ostringstream()
            << "Opened ROM " << path << " with size " << buffer_size
          )
            .str()
            .c_str()
        );

        m64p_check(get_fn<"CoreDoCommand">(
        )(M64CMD_ROM_OPEN, buffer_size, buffer.get()));
      }
    }

    // Closes the currently open ROM.
    void close_rom() {
      m64p_check(get_fn<"CoreDoCommand">()(M64CMD_ROM_CLOSE, 0, nullptr));
    }

    // Starts Mupen64Plus synchronously. You can use C++'s threading utilities
    // to do so asynchronously.
    void run_sync() {
      m64p_check(get_fn<"CoreDoCommand">()(M64CMD_EXECUTE, 0, nullptr));
    }

    // Stops the emulator. This does not unload the ROM.
    void stop() {
      m64p_check(get_fn<"CoreDoCommand">()(M64CMD_STOP, 0, nullptr));
    }
    // Pauses the emulator.
    void pause() {
      m64p_check(get_fn<"CoreDoCommand">()(M64CMD_PAUSE, 0, nullptr));
    }
    // Resumes the emulator.
    void resume() {
      m64p_check(get_fn<"CoreDoCommand">()(M64CMD_RESUME, 0, nullptr));
    }

    // Resets the emulator.
    // If the parameter `hard` is true, performs a hard reset. Otherwise, does a
    // soft reset.
    void reset(bool hard = false) {
      m64p_check(get_fn<"CoreDoCommand">()(M64CMD_RESET, hard, nullptr));
    }
    // Advances the emulator one frame.
    void frame_advance() {
      m64p_check(get_fn<"CoreDoCommand">()(M64CMD_ADVANCE_FRAME, 0, nullptr));
    }

    // Attaches a plugin, loaded from a path.
    // Auto-determines type using PluginGetVersion.
    void load_plugin(const char* path) {
      m64p_dynlib_handle plugin = oslib::pdlopen(path);

      // This auto-detects the plugin type to make sure the user isn't stupid
      m64p_plugin_type type = M64PLUGIN_NULL;
      M64P_LDFN(plugin, PluginGetVersion)
      (&type, nullptr, nullptr, nullptr, nullptr);
      if (plugins.contains(type)) {
        oslib::pdlclose(plugin);
        throw std::runtime_error("Plugin already loaded for this type!");
      }

      m64p_check(M64P_LDFN(
        plugin, PluginStartup
      )(lib_handle, this, [](void* self, int level, const char* msg) {
        reinterpret_cast<core*>(self)->debug_log(
          static_cast<m64p_msg_level>(level), msg
        );
      }));
      m64p_check(get_fn<"CoreAttachPlugin">()(type, plugin));
      plugins[type] = plugin;
    }

    // Detaches a plugin by type.
    void unload_plugin(m64p_plugin_type type) {
      m64p_dynlib_handle plugin = plugins.at(type);

      m64p_check(get_fn<"CoreDetachPlugin">()(type));
      m64p_check(M64P_LDFN(plugin, PluginShutdown)());
      oslib::pdlclose(plugin);
      plugins.erase(type);
    }

    // Rerecording hooks
    void vcr_start_movie(const char* path) { get_fn<"VCR_StartMovie">()(path); }
    void vcr_stop_movie(bool loop = false) { get_fn<"VCR_StopMovie">()(loop); }
    bool vcr_is_readonly() { return get_fn<"VCR_IsReadOnly">()(); }
    bool vcr_is_playing() { return get_fn<"VCR_IsPlaying">()(); }
    bool vcr_set_readonly(bool x) { return get_fn<"VCR_SetReadOnly">()(x); }
    uint32_t vcr_current_frame() { return get_fn<"VCR_GetCurFrame">()(); }

    bool enc_is_active() { return get_fn<"Encoder_IsActive">(); }
    void enc_start(const char* path, m64p_encoder_format fmt = M64FMT_NULL) {
      m64p_check(get_fn<"Encoder_Start">()(path, fmt));
    }
    void enc_stop(bool discard = false) {
      m64p_check(get_fn<"Encoder_Stop">()(discard));
    }

  private:
    using fn_table_t = ntuple<
      // Startup/Shutdown
      M64P_NTFN(CoreStartup), M64P_NTFN(CoreShutdown),
      // Plugin handling
      M64P_NTFN(CoreAttachPlugin), M64P_NTFN(CoreDetachPlugin),
      // Misc core stuff
      M64P_NTFN(CoreDoCommand), M64P_NTFN(CoreOverrideVidExt),
      M64P_NTFN(CoreErrorMessage),
      // Re-recording hooks
      M64P_NTFN(VCR_GetCurFrame), M64P_NTFN(VCR_StopMovie),
      M64P_NTFN(VCR_SetKeys), M64P_NTFN(VCR_GetKeys), M64P_NTFN(VCR_IsPlaying),
      M64P_NTFN(VCR_IsReadOnly), M64P_NTFN(VCR_SetReadOnly),
      M64P_NTFN(VCR_StartRecording), M64P_NTFN(VCR_StartMovie),
      M64P_NTFN(VCR_SetErrorCallback),
      // Encoder hooks
      M64P_NTFN(Encoder_IsActive), M64P_NTFN(Encoder_Start),
      M64P_NTFN(Encoder_Stop)>;

    using handler_list_t = std::list<fnptr<bool(m64p_core_param, int)>>;

    m64p_dynlib_handle lib_handle;
    std::unique_ptr<fn_table_t> fn_table;
    std::unordered_map<m64p_plugin_type, m64p_dynlib_handle> plugins;
    handler_list_t handlers;

    // Handle state changes. I used a linked list because it avoids
    // resizing problems with vectors.
    void run_state_handlers(m64p_core_param p, int nv) {
      auto it1 = handlers.begin();
      while (it1 != handlers.end()) {
        bool res = (*it1)(p, nv);
        if (!res)
          it1 = handlers.erase(it1);
        else
          it1++;
      }
    }

    // Implementation utilities
    // ====================

    static fn_table_t* init_fn_table(m64p_dynlib_handle hnd) {
      // Use an IIFE to expand the parameter packs
      return [&]<fixed_string... Ks, class... Ts>(std::type_identity<
                                                  ntuple<nt_leaf<Ks, Ts>...>>) {
        // Ensure immediate initialization
        return new fn_table_t {oslib::pdlsym<Ts>(hnd, Ks.c_str())...};
      }
      (std::type_identity<fn_table_t> {});
    }

    template <fixed_string K>
    inline ntuple_element_t<K, fn_table_t> get_fn() {
      return get<K>(*fn_table.get());
    }

    void m64p_check(m64p_error err) {
      if (err != M64ERR_SUCCESS) {
        throw std::runtime_error(get_fn<"CoreErrorMessage">()(err));
      }
    }
  };
}  // namespace m64p
#undef M64P_NTFN
#undef M64P_LDFN
#endif