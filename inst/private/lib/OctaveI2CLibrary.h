#include "LibraryBase.h"

class OctaveI2CLibrary : public LibraryBase
{
public:
   OctaveI2CLibrary(OctaveArduinoClass &oc);
   void commandHandler(uint8_t cmdID, uint8_t* inputs, uint8_t payload_size);
};


