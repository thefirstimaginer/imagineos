#pragma once

// Definição dos Registros
#define RTC_REGISTER_MINUTES 0x02
#define RTC_REGISTER_HOURS   0x04
#define RTC_REGISTER_DAY     0x07
#define RTC_REGISTER_MONTH   0x08
#define RTC_REGISTER_YEAR    0x09


uint8_t rtc_seconds();

// Protótipos das funções (Diz ao compilador que elas existem)
uint8_t rtc_read_register(uint8_t reg);
void rtc_wait();
uint8_t rtc_is_bcd();
uint8_t rtc_get_value(uint8_t reg); // A função que criamos