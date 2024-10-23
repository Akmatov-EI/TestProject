#include "parser.h"
#include <stdio.h>
#include <string.h>

// Пример коллбека для приёма пакетов
void packet_received_callback(int uart_id, uint16_t packet_type, const uint8_t *payload, size_t payload_size) {
    printf("UART %d: Принят пакет. Тип: 0x%04X, Размер: %zu байт, Данные: ", uart_id, packet_type, payload_size);
    for(size_t i = 0; i < payload_size; i++) {
        printf("0x%02X ", payload[i]);
    }
    printf("\n");
}

int main() {
    // Определение синхропоследовательности
    uint8_t sync_seq[] = {0xAA, 0x55};

    // Инициализация UART-протокола для UART0
    if (uart_protocol_init(0, sync_seq, sizeof(sync_seq), packet_received_callback) != 0) {
        printf("Ошибка инициализации UART-протокола.\n");
        return -1;
    }

    // Формирование и отправка пакета
    uint8_t payload[] = {0x10, 0x20, 0x30, 0x40};
    if (uart_protocol_send_packet(0, 0x1001, payload, sizeof(payload)) != 0) {
        printf("Ошибка отправки пакета.\n");
    }

    // Симуляция получения данных
    uint8_t received_data[] = {0xAA, 0x55, 0x04, 0x10, 0x20, 0x30, 0x40};
    uart_protocol_receive_data(0, received_data, sizeof(received_data));

    return 0;
}
