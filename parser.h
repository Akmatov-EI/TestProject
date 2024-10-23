#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Тип коллбека, вызываемого при приёме валидного пакета.
 *
 * @param uart_id Идентификатор UART-интерфейса.
 * @param packet_type Тип принятого пакета.
 * @param payload Указатель на тело пакета.
 * @param payload_size Размер тела пакета в байтах.
 */
typedef void (*uart_packet_callback_t)(int uart_id, uint16_t packet_type, const uint8_t *payload, size_t payload_size);

/**
 * @brief Инициализация UART-протокола.
 *
 * @param uart_id Идентификатор UART-интерфейса.
 * @param sync_seq Указатель на синхропоследовательность.
 * @param sync_len Длина синхропоследовательности.
 * @param callback Коллбек для приёма пакетов.
 * @return 0 в случае успеха, иначе отрицательное значение.
 */
int uart_protocol_init(int uart_id, const uint8_t *sync_seq, size_t sync_len, uart_packet_callback_t callback);

/**
 * @brief Отправка пакета данных через UART.
 *
 * @param uart_id Идентификатор UART-интерфейса.
 * @param packet_type Тип пакета.
 * @param payload Указатель на тело пакета.
 * @param payload_size Размер тела пакета в байтах.
 * @return 0 в случае успеха, иначе отрицательное значение.
 */
int uart_protocol_send_packet(int uart_id, uint16_t packet_type, const uint8_t *payload, size_t payload_size);

/**
 * @brief Обработка входящих данных UART.
 *
 * @param uart_id Идентификатор UART-интерфейса.
 * @param data Указатель на входящие данные.
 * @param length Количество байтов во входящих данных.
 */
void uart_protocol_receive_data(int uart_id, const uint8_t *data, size_t length);

#endif // UART_PROTOCOL_H
