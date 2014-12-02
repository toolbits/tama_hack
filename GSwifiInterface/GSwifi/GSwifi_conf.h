
//#define DEBUG
//#define DEBUG_DUMP

//#define CFG_ENABLE_RTOS
#define CFG_ENABLE_HTTPD
//#define CFG_ENABLE_WEBSOCKET
//#define CFG_ENABLE_SMTP
#define CFG_UART_DIRECT

#define CFG_UART_BAUD 9600
#define CFG_WREGDOMAIN 2 // 0:FCC, 1:ETSI, 2:TELEC
#define CFG_DHCPNAME "mbed-gswifi"
#define CFG_DNSNAME "setup.local"
#define CFG_TRYJOIN 3
#define CFG_RECONNECT 30 // sec
#define CFG_MAX_SOCKETS 16

#define DEFAULT_WAIT_RESP_TIMEOUT 500
#define CFG_TIMEOUT 15000 // ms

#define CFG_CMD_SIZE 128

#if defined(TARGET_LPC1768) || defined(TARGET_LPC2368) || defined(TARGET_LPC176X)
#define CFG_DATA_SIZE 1024
#elif defined(TARGET_LPC11U24) || defined(TARGET_LPC1114) || defined(TARGET_LPC11UXX) || defined(TARGET_LPC11XX)
#define CFG_DATA_SIZE 256
#elif defined(TARGET_LPC812) || defined(TARGET_LPC81X)
#define CFG_DATA_SIZE 128
#elif defined(TARGET_LPC4088) || defined(TARGET_LPC408X)
#define CFG_DATA_SIZE 4096
#elif defined(TARGET_KL25Z)
#define CFG_DATA_SIZE 512
#endif
/*
#if defined(TARGET_LPC1768) || defined(TARGET_LPC176X)
#define CFG_EXTENDED_MEMORY1 0x2007c000
#define CFG_EXTENDED_SIZE1 0x4000
#define CFG_EXTENDED_MEMORY2 0x20082000
#define CFG_EXTENDED_SIZE2 0x2000
#endif
*/
#define CFG_HTTPD_HANDLER_NUM 10
#define CFG_HTTPD_KEEPALIVE 10
