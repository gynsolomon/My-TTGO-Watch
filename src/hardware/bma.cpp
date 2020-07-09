#include "config.h"
#include <TTGO.h>
#include <soc/rtc.h>

#include "bma.h"
#include "powermgm.h"

#include "gui/statusbar.h"

EventGroupHandle_t bma_event_handle = NULL;

void IRAM_ATTR  bma_irq( void );

bma_config_t bma_config[ BMA_CONFIG_NUM ];

void bma_reload_settings( void );
void bma_read_config( void );
void bma_save_config( void );

/*
 *
 */
void bma_setup( TTGOClass *ttgo ) {

    bma_event_handle = xEventGroupCreate();

    for ( int i = 0 ; i < BMA_CONFIG_NUM ; i++ ) {
        bma_config[ i ].enable = true;
    }

    bma_read_config();

    ttgo->bma->begin();
    ttgo->bma->attachInterrupt();
    ttgo->bma->direction();

    pinMode( BMA423_INT1, INPUT );
    attachInterrupt( BMA423_INT1, bma_irq, RISING );

    ttgo->bma->enableStepCountInterrupt( bma_config[ BMA_STEPCOUNTER ].enable );
    ttgo->bma->enableWakeupInterrupt( bma_config[ BMA_DOUBLECLICK ].enable );
}

/*
 *
 */
void bma_reload_settings( void ) {

    TTGOClass *ttgo = TTGOClass::getWatch();

    ttgo->bma->enableStepCountInterrupt( bma_config[ BMA_STEPCOUNTER ].enable );
    ttgo->bma->enableWakeupInterrupt( bma_config[ BMA_DOUBLECLICK ].enable );
}

/*
 *
 */
void IRAM_ATTR  bma_irq( void ) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xEventGroupSetBitsFromISR( bma_event_handle, BMA_EVENT_INT, &xHigherPriorityTaskWoken );

    if ( xHigherPriorityTaskWoken )
    {
        portYIELD_FROM_ISR ();
    }
    rtc_clk_cpu_freq_set( RTC_CPU_FREQ_240M );
    // setCpuFrequencyMhz(240);
}

/*
 * loop routine for handling IRQ in main loop
 */
void bma_loop( TTGOClass *ttgo ) {
    /*
     * handle IRQ event
     */
    if ( xEventGroupGetBitsFromISR( bma_event_handle ) & BMA_EVENT_INT ) {
        while( !ttgo->bma->readInterrupt() );
        if ( ttgo->bma->isDoubleClick() ) {
            powermgm_set_event( POWERMGM_BMA_WAKEUP );
            xEventGroupClearBitsFromISR( bma_event_handle, BMA_EVENT_INT );
            return;
        }
    }

    if ( !powermgm_get_event( POWERMGM_STANDBY ) && xEventGroupGetBitsFromISR( bma_event_handle ) & BMA_EVENT_INT ) {
        statusbar_update_stepcounter( ttgo->bma->getCounter() );
        xEventGroupClearBitsFromISR( bma_event_handle, BMA_EVENT_INT );
    }
}

/*
 *
 */
void bma_save_config( void ) {
  fs::File file = SPIFFS.open( BMA_COFIG_FILE, FILE_WRITE );

  if ( !file ) {
    Serial.printf("Can't save file: %s\r\n", BMA_COFIG_FILE );
  }
  else {
    file.write( (uint8_t *)bma_config, sizeof( bma_config ) );
    file.close();
  }
}

/*
 *
 */
void bma_read_config( void ) {
  fs::File file = SPIFFS.open( BMA_COFIG_FILE, FILE_READ );

  if (!file) {
    Serial.printf("Can't open file: %s!\r\n", BMA_COFIG_FILE );
  }
  else {
    int filesize = file.size();
    if ( filesize > sizeof( bma_config ) ) {
      Serial.printf("Failed to read configfile. Wrong filesize!\r\n" );
    }
    else {
      file.read( (uint8_t *)bma_config, filesize );
    }
    file.close();
  }
}

/*
 *
 */
bool bma_get_config( int config ) {
    if ( config < BMA_CONFIG_NUM ) {
        return( bma_config[ config ].enable );
    }
    return false;
}

/*
 *
 */
void bma_set_config( int config, bool enable ) {
    if ( config < BMA_CONFIG_NUM ) {
        bma_config[ config ].enable = enable;
        bma_save_config();
        bma_reload_settings();
    }
}