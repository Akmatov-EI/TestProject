#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Максимальный размер полезных данных
#define UART_PROTOCOL_MAX_PAYLOAD 1000

// Структура FIFO-буфера
typedef struct {
    uint8_t *buffer;
    size_t head;
    size_t tail;
    size_t size;
} fifo_t;

// Структура состояния UART
typedef struct uart_state {
    int uart_id;
    const uint8_t *sync_seq;
    size_t sync_len;
    uart_packet_callback_t callback;
    fifo_t fifo;

    // Парсинг состояния
    size_t sync_match;
    uint16_t payload_size;
    uint16_t packet_type;
    size_t header_bytes;
    uint8_t header_checksum;
    uint8_t payload_checksum[2];
    uint8_t calculated_checksum[2];
    uint8_t *payload_buffer;
    size_t payload_received;
    int in_packet;
    struct uart_state *next;
} uart_state_t;

// Максимальное количество UART
#define MAX_UART 10
static uart_state_t *uart_states[MAX_UART] = {0};

// Простейшая реализация контрольной суммы (сумма байтов)
static uint16_t calculate_checksum(const uint8_t *data, size_t length) {
    uint16_t sum = 0;
    for(size_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

// Инициализация FIFO
static int fifo_init(fifo_t *fifo, size_t size) {
    fifo->buffer = (uint8_t*)malloc(size);
    if (!fifo->buffer) return -1;
    fifo->head = fifo->tail = 0;
    fifo->size = size;
    return 0;
}

// Запись в FIFO
static int fifo_write(fifo_t *fifo, const uint8_t *data, size_t length) {

    if (length > fifo->size - 1) return -1; // Переполнение
    for(size_t i = 0; i < length; i++) {
        size_t next = (fifo->head + 1) % fifo->size;
        if (next == fifo->tail) return -1; // Переполнение
        fifo->buffer[fifo->head] = data[i];
        fifo->head = next;
    }
    return 0;
}

// Чтение из FIFO
static int fifo_read(fifo_t *fifo, uint8_t *data) {
    if (fifo->head == fifo->tail) return -1; // Пусто
    *data = fifo->buffer[fifo->tail];
    fifo->tail = (fifo->tail + 1) % fifo->size;
    return 0;
}

// Очистка FIFO
static void fifo_clear(fifo_t *fifo) {
    fifo->head = fifo->tail = 0;
}

// Поиск и обработка пакетов
static void process_uart(uart_state_t *state) {
    uint8_t byte;
    while (fifo_read(&state->fifo, &byte) == 0) {
        if (!state->in_packet) {
            // Поиск синхропоследовательности
            if (byte == state->sync_seq[state->sync_match]) {
                state->sync_match++;
                if (state->sync_match == state->sync_len) {
                    state->in_packet = 1;
                    state->sync_match = 0;
                    // Инициализация состояния парсинга заголовка
                    state->header_bytes = 0;
                    state->payload_size = 0;
                    state->packet_type = 0;
                    state->header_checksum = 0;
                    fifo_clear(&state->fifo); // Очистка буфера до начала тела пакета
                }
            } else {
                if (state->sync_match > 0) {
                    // Частичное совпадение
                    state->sync_match = 0;
                    if (byte == state->sync_seq[0]) {
                        state->sync_match = 1;
                    }
                }
            }
        } else {
            // В стадии приема после синхропоследовательности
            // Для упрощения примера пропустим подробный парсинг заголовка
            // Предположим, что заголовок фиксированной длины

            // TODO: Реализовать парсинг заголовка согласно спецификации

            // Для демонстрации вызовем коллбек с фиктивными данными
            if (state->callback) {
                uint8_t dummy_payload[] = {0x01, 0x02, 0x03};
                state->callback(state->uart_id, 0x1234, dummy_payload, sizeof(dummy_payload));
            }
            state->in_packet = 0;
        }
    }
}

int uart_protocol_init(int uart_id, const uint8_t *sync_seq, size_t sync_len, uart_packet_callback_t callback) {
    if (uart_id < 0 || uart_id >= MAX_UART) return -1;
    if (uart_states[uart_id]) return -1; // Уже инициализирован
    uart_state_t *state = (uart_state_t*)malloc(sizeof(uart_state_t));
    if (!state) return -1;
    state->uart_id = uart_id;
    state->sync_seq = sync_seq;
    state->sync_len = sync_len;
    state->callback = callback;
    if (fifo_init(&state->fifo, 2048) != 0) {
        free(state);
        return -1;
    }
    state->sync_match = 0;
    state->in_packet = 0;
    state->payload_buffer = NULL;
    state->next = NULL;
    uart_states[uart_id] = state;
    return 0;
}

int encode_variable_length(uint16_t value, uint8_t *encoded, size_t *encoded_len) {
    if (value < 128) {
        encoded[0] = value & 0x7F;
        *encoded_len = 1;
    } else if (value < 32768) { // Так как используем 2 байта
        encoded[0] = (value - 128) & 0x7F;
        encoded[1] = (value >> 7) & 0xFF;
        *encoded_len = 2;
    } else {
        return -1; // Значение слишком большое
    }
    return 0;
}

int uart_protocol_send_packet(int uart_id, uint16_t packet_type, const uint8_t *payload, size_t payload_size) {
    if (uart_id < 0 || uart_id >= MAX_UART) return -1;
    uart_state_t *state = uart_states[uart_id];
    if (!state) return -1;

    // Формирование заголовка
    uint8_t header[7];
    size_t header_len = 0;

    // Размер полезных данных
    size_t size_encoded_len;
    uint8_t size_encoded[2];
    if (encode_variable_length(payload_size, size_encoded, &size_encoded_len) != 0) return -1;
    memcpy(header + header_len, size_encoded, size_encoded_len);

    header_len += size_encoded_len;

    // Тип содержимого
    size_t type_encoded_len;
    uint8_t type_encoded[2];
    if (encode_variable_length(packet_type, type_encoded, &type_encoded_len) != 0) return -1;
    memcpy(header + header_len, type_encoded, type_encoded_len);
    header_len += type_encoded_len;

    // Контрольная сумма заголовка
    if (payload_size > 0) {
        uint16_t checksum = calculate_checksum(payload, payload_size);
        if (payload_size < 32) {
            header[header_len++] = (uint8_t)(checksum & 0xFF);
        } else {
            header[header_len++] = (uint8_t)(checksum & 0xFF);
            header[header_len++] = (uint8_t)((checksum >> 8) & 0xFF);
        }
    }

    // Формирование полного пакета
    size_t total_len = state->sync_len + header_len + payload_size;
    uint8_t *packet = (uint8_t*)malloc(total_len);
    if (!packet) return -1;

    // Синхропоследовательность
    memcpy(packet, state->sync_seq, state->sync_len);

    // Заголовок
    memcpy(packet + state->sync_len, header, header_len);

    // Тело пакета
    memcpy(packet + state->sync_len + header_len, payload, payload_size);

    // Отправка пакета (для примера используем printf)
    printf("UART %d: Отправка пакета: ", uart_id);
    for(size_t i = 0; i < total_len; i++) {
        printf("0x%02X ", packet[i]);
    }
    printf("\n");

    free(packet);
    return 0;
}

void uart_protocol_receive_data(int uart_id, const uint8_t *data, size_t length) {
    if (uart_id < 0 || uart_id >= MAX_UART) return;
    uart_state_t *state = uart_states[uart_id];
    if (!state) return;

    // Запись данных в FIFO
    if (fifo_write(&state->fifo, data, length) != 0) {
        // Переполнение FIFO, сброс состояния
        fifo_clear(&state->fifo);
        state->in_packet = 0;
        state->sync_match = 0;
        return;
    }

    // Обработка данных
    process_uart(state);
}