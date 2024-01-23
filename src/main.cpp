#include "config/paths.hpp"
#include "m64pp/core.hpp"
#include "mupen64plus/m64p_types.h"

#include <cxxabi.h>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <ostream>
#include <thread>
#include <typeinfo>
using namespace std::literals;
using std::cerr;

std::thread emu_thread(m64p::core& core) {
  return std::thread([&]() { core.run_sync(); });
}

std::string get_type_name(const std::type_info& ti) {
#if __GNUC__
  int stat         = -69420;
  const char* name = ti.name();

  std::unique_ptr<char, void (*)(void*)> res {
    abi::__cxa_demangle(name, NULL, NULL, &stat), std::free};

  return (stat == 0) ? res.get() : name;
#else
  return ti.name();
#endif
}

void log_exception(const std::exception& e, bool nested = false) {
  if (nested)
    cerr << "Caused by: ";
  cerr << get_type_name(typeid(e)) << '\n';
  cerr << "  "sv << e.what() << '\n';

  try {
    std::rethrow_if_nested(e);
  }
  catch (const std::exception& ne) {
    log_exception(ne, true);
  }
}

void on_terminate() {
  try {
    auto eptr = std::current_exception();
    if (eptr)
      std::rethrow_exception(eptr);
  }
  catch (const std::exception& e) {
    log_exception(e);
  }
  catch (...) {
    
  }
  abort();
}

int main() {
  std::set_terminate(on_terminate);

  m64p::core c(M64P_PATH_CORE);
  c.open_rom(M64P_PATH_ROM);

  c.load_plugin(M64P_PATH_VIDEO);
  c.load_plugin(M64P_PATH_AUDIO);
  c.load_plugin(M64P_PATH_INPUT);
  c.load_plugin(M64P_PATH_RSP);

  auto t = emu_thread(c);

  std::this_thread::sleep_for(3s);
  c.vcr_start_movie(M64P_PATH_MOVIE);
  // c.enc_start("out.mp4");

  int emu_state;
#if 1
  while (c.vcr_is_playing()) {
    std::this_thread::sleep_for(500ms);
  }
  std::this_thread::sleep_for(20s);
  c.stop();
#else
  std::this_thread::sleep_for(10s);
  c.stop();
#endif
  // c.enc_stop();

  if (t.joinable())
    t.join();

  c.close_rom();

  c.unload_plugin(M64PLUGIN_GFX);
  c.unload_plugin(M64PLUGIN_AUDIO);
  c.unload_plugin(M64PLUGIN_INPUT);
  c.unload_plugin(M64PLUGIN_RSP);
}