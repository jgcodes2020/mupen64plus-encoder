#include "m64pp/core.hpp"
#include "config/paths.hpp"
#include "mupen64plus/m64p_types.h"

#include <thread>
using namespace std::literals;

std::thread emu_thread(m64p::core& core) {
  return std::thread([&]() {
    core.run_sync();
  });
}

int main() {
  m64p::core c(M64P_PATH_CORE);
  c.open_rom(M64P_PATH_ROM);
  
  c.load_plugin(M64P_PATH_VIDEO);
  c.load_plugin(M64P_PATH_AUDIO);
  c.load_plugin(M64P_PATH_INPUT);
  c.load_plugin(M64P_PATH_RSP);
  
  auto t = emu_thread(c);
  
  c.vcr_start_movie(M64P_PATH_MOVIE);
  c.enc_start("out.webm");
  
  while (c.vcr_is_playing()) {
    std::this_thread::sleep_for(100ms);
  }
  c.enc_stop();
  c.stop();
  
  if (t.joinable()) t.join();
  
  c.close_rom();
  
  c.unload_plugin(M64PLUGIN_GFX);
  c.unload_plugin(M64PLUGIN_AUDIO);
  c.unload_plugin(M64PLUGIN_INPUT);
  c.unload_plugin(M64PLUGIN_RSP);
}