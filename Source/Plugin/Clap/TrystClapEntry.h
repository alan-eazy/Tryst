#pragma once

extern "C" {
bool tryst_clap_entry_init(const char* pluginPath);
void tryst_clap_entry_deinit();
const void* tryst_clap_entry_get_factory(const char* factoryId);
}
