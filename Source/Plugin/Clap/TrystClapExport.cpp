#include <clap/clap.h>
#include "TrystClapEntry.h"

extern "C" {
const CLAP_EXPORT clap_plugin_entry_t clap_entry = {
    CLAP_VERSION,
    tryst_clap_entry_init,
    tryst_clap_entry_deinit,
    tryst_clap_entry_get_factory
};
}
