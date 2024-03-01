#include "dummy_config.h"

// The primary use-case of this is to let the undumper passthrough a filename via sigProp.jsonConfig to the undumper dll

// Get the signal properties, to know what data to expect
DYNALO_EXPORT void DYNALO_CALL GetSignalProperties(sig::SignalProperties& sigProp)
{
	

}

