#include "m64pp/core.hpp"
#include "config/paths.hpp"
#include "mupen64plus/m64p_types.h"

int main() {
  m64p::core c(M64P_PATH_CORE);
  c.open_rom(M64P_PATH_ROM);
  
  c.load_plugin(M64P_PATH_VIDEO);
  c.load_plugin(M64P_PATH_AUDIO);
  c.load_plugin(M64P_PATH_INPUT);
  c.load_plugin(M64P_PATH_RSP);
  
  c.run_sync();
  
  c.close_rom();
  
  c.unload_plugin(M64PLUGIN_GFX);
  c.unload_plugin(M64PLUGIN_AUDIO);
  c.unload_plugin(M64PLUGIN_INPUT);
  c.unload_plugin(M64PLUGIN_RSP);
}