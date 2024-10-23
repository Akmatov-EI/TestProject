# Минимальная требуемая версия CMake
cmake_minimum_required(VERSION 3.17)

# Название проекта и используемый язык
project(PARSER_UART C)

# Установка стандарта языка C
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Определение максимального размера пакета через опцию компилятора
add_definitions(-DUART_PROTOCOL_MAX_PAYLOAD=1000)

# Создание статической библиотеки из модульных исходных файлов
add_library(parser STATIC parser.c)

# Указание каталогов для поиска заголовочных файлов
target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Добавление исполняемого файла из главного файла
add_executable(main main.c)


target_link_libraries(main PRIVATE uart_protocol)


if (CMAKE_COMPILER_IS_GNUCC)
    target_compile_options(uart_protocol PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(main PRIVATE -Wall -Wextra -pedantic)
elseif (MSVC)
    target_compile_options(uart_protocol PRIVATE /W4)
    target_compile_options(main PRIVATE /W4)
endif()


install(TARGETS main uart_protocol
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)