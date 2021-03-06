#include "config.h"
#include "ArduinoJson.h"

// arduinoJson allocator for external PSRAM
// see: https://arduinojson.org/v6/how-to/use-external-ram-on-esp32/
struct SpiRamAllocator {
    void* allocate( size_t size ) { 
        void *psram = ps_calloc( size, 1 );
        if ( psram ) {
            log_i("allocate %d bytes (%p) json psram", size, psram );
            return( psram );
        }
        else {
            log_e("allocate %d bytes (%p) json psram failed", size, psram );
            while(1);
        }
    }
    void deallocate( void* pointer ) {
        log_i("deallocate (%p) json psram", pointer );
        free( pointer );
    }
};
using SpiRamJsonDocument = BasicJsonDocument<SpiRamAllocator>;