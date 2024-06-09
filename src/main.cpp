#include "main.h"
#include <lvgl.h>
#include <DHT.h>
#include "display_mng.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Macros
#define DHT_PIN                             (33)
#define DHT_TYPE                            (DHT22)
#define LVGL_REFRESH_TIME                   (5u)      // 5 milliseconds
#define DHT_REFRESH_TIME                    (2000u)   // 2 seconds

// Task priorities
#define DHT_TASK_PRIORITY                   (configMAX_PRIORITIES - 1)
#define DISPLAY_TASK_PRIORITY               (configMAX_PRIORITIES - 2)
#define LVGL_TASK_PRIORITY                  (configMAX_PRIORITIES - 3)

// Private Variables
static uint32_t lvgl_refresh_timestamp = 0u;
static uint32_t dht_refresh_timestamp = 0u;

// DHT Related Variables and Instances
DHT dht(DHT_PIN, DHT_TYPE);
static Sensor_Data_s sensor_data = 
{
  .sensor_idx = 0u,
};

// Task handles
TaskHandle_t DHT_TaskHandle = NULL;
TaskHandle_t Display_TaskHandle = NULL;
TaskHandle_t LVGL_TaskHandle = NULL;

// Private functions
static void DHT_TaskInit( void );
static void DHT_TaskMng( void *pvParameters );
static void LVGL_TaskInit( void );
static void LVGL_TaskMng( void *pvParameters );
static void Display_TaskMng( void *pvParameters );

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial.println("Temperature and Humidity Graph Using LVGL");

  // Intialize the LVGL Library
  LVGL_TaskInit();
  
  // Initialize Display and Display Buffers
  Display_Init();

  DHT_TaskInit();
  
  // Create tasks for FreeRTOS
  xTaskCreatePinnedToCore(DHT_TaskMng, "DHT Task", 2048, NULL, DHT_TASK_PRIORITY, &DHT_TaskHandle, 1);
  xTaskCreatePinnedToCore(Display_TaskMng, "Display Task", 2048, NULL, DISPLAY_TASK_PRIORITY, &Display_TaskHandle, 1);
  xTaskCreatePinnedToCore(LVGL_TaskMng, "LVGL Task", 2048, NULL, LVGL_TASK_PRIORITY, &LVGL_TaskHandle, 1);
}

void loop()
{
  // Nothing to do here, everything is handled by tasks
}

Sensor_Data_s * Get_TemperatureAndHumidity( void )
{
  return &sensor_data;
}

// Private Function Definition
static void DHT_TaskInit( void )
{
  dht.begin();
  dht_refresh_timestamp = millis();
}

static void DHT_TaskMng( void *pvParameters )
{
  while (1)
  {
    uint32_t now = millis();
    float temperature = 0.0;
    float humidity = 0.0;
    if( (now - dht_refresh_timestamp) >= DHT_REFRESH_TIME )
    {
      dht_refresh_timestamp = now;
      // Reading temperature or humidity takes about 250 milliseconds!
      // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
      humidity = dht.readHumidity();
      // Read temperature as Celsius (the default)
      temperature = dht.readTemperature();
      
      // Check if any reads failed and exit early (to try again).
      if (isnan(humidity) || isnan(temperature) ) 
      {
        Serial.println(F("Failed to read from DHT sensor!"));
      }
      else
      {
        Serial.print(F("Humidity: "));
        Serial.print(humidity);
        Serial.print(F("%  Temperature: "));
        Serial.print(temperature);
        Serial.println(F("°C "));
        if( sensor_data.sensor_idx < SENSOR_BUFF_SIZE )
        {
          sensor_data.temperature[sensor_data.sensor_idx] = (uint8_t)(temperature);
          sensor_data.humidity[sensor_data.sensor_idx] = (uint8_t)(humidity);
          sensor_data.sensor_idx++;
          // Reset to Zero
          if( sensor_data.sensor_idx >= SENSOR_BUFF_SIZE )
          {
            sensor_data.sensor_idx = 0u;
          }
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100)); // Yield to other tasks
  }
}

static void LVGL_TaskInit( void )
{
  lv_init();
  lvgl_refresh_timestamp = millis();
}

static void LVGL_TaskMng( void *pvParameters )
{
  while (1)
  {
    uint32_t now = millis();
    // LVGL Refresh Timed Task
    if( (now - lvgl_refresh_timestamp) >= LVGL_REFRESH_TIME )
    {
      lvgl_refresh_timestamp = now;
      // let the GUI does work
      lv_timer_handler();
    }
    vTaskDelay(pdMS_TO_TICKS(5)); // Yield to other tasks
  }
}

static void Display_TaskMng( void *pvParameters )
{
  while (1)
  {
    Display_Mng();
    vTaskDelay(pdMS_TO_TICKS(100)); // Yield to other tasks
  }
}
