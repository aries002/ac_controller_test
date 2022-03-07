
#define PIN_POWER 0
#define PIN_SIGNAL 13

#ifdef BOARD_V1
#define PIN_SDA 12
#define PIN_SCL 14
#define PZEM_RX_PIN 4
#define PZEM_TX_PIN 5
#endif

#ifdef BOARD_V3
#define PIN_SDA 4
#define PIN_SCL 5
#define PZEM_RX_PIN 12
#define PZEM_TX_PIN 14
#endif
#define AC_READ_TIMEOUT 3000

#define WIFI_SSID "smartcampus@umsida.ac.id"
#define WIFI_PASSWORD "umsida1912"

#define SERVER_HOST "iot.umsida.ac.id"
#define SERVER_TOKEN "6ed4b761d22f2a611ac664933ab76da121a66b89"
#define TOKEN "1234567890"

#define FIELD_SENSOR_SUHU 0
#define FIELD_SENSOR_KELEMBAPAN 1
#define FIELD_STATUS_AC 2
#define FIELD_COMMAND_AC 3
#define FIELD_SENSOR_TEGANGAN 4
#define FIELD_SENSOR_ARUS 5
#define FIELD_SENSOR_DAYA 6
#define FIELD_SENSOR_FREKUENSI 7
#define FIELD_SENSOR_PF 8
#define FIELD_LOCATE 9
