
/**
 * @file    bignum_init_from_string.h
 * @brief   Инициализация bignum_t из строки в заданном или автоматически выбранном основании.
 */
/* ------------------------------------------------------------------ */
#pragma once
#ifndef BIGNUM_INIT_FROM_STRING_H
#define BIGNUM_INIT_FROM_STRING_H

#include <stddef.h>
#include <stdint.h>

#include "bignum.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Результаты выполнения bignum_init_from_string. */
typedef enum {
    BIGNUM_INIT_FROM_STRING_SUCCESS        = 0,
    BIGNUM_INIT_FROM_STRING_ERROR_NULL_ARG = -1,
    BIGNUM_INIT_FROM_STRING_ERROR_OVERFLOW = -2,
    BIGNUM_INIT_FROM_STRING_ERROR_BAD_BASE = -3,
    BIGNUM_INIT_FROM_STRING_ERROR_EMPTY    = -4,
    BIGNUM_INIT_FROM_STRING_ERROR_PARSE    = -5
} bignum_init_from_string_status_t;

/**
 * @brief Инициализирует bignum_t из строки.
 *
 * Ведущие ASCII-пробелы пропускаются. Один ведущий знак '+' или '-' также
 * пропускается и не влияет на беззнаковый результат. После знака допускаются
 * ведущие ASCII-пробелы.
 *
 * Поддерживаются основания от 2 до 16 и цифры 0-9, a-f, A-F. При base == 0
 * префиксы 0b/0B, 0o/0O и 0x/0X выбирают основания 2, 8 и 16 соответственно;
 * без префикса используется основание 10. При base == 2, 8 или 16
 * соответствующий префикс также снимается автоматически. Для других явных
 * оснований префиксы не разрешаются.
 *
 * Вся строка после знака и префикса проверяется посимвольно. Недопустимая
 * цифра, внутренний или завершающий пробел, лишний знак и неподдерживаемый
 * префикс приводят к ошибке разбора. Пустой остаток после знака или префикса
 * приводит к BIGNUM_INIT_FROM_STRING_ERROR_EMPTY.
 *
 * При успешном вызове результат записывается в dst. При NULL-аргументе,
 * недопустимом основании, пустой строке, недопустимой цифре или переполнении
 * функция немедленно возвращает соответствующий статус. При ошибке состояние
 * dst не следует использовать как корректно инициализированное значение.
 *
 * @param[out] dst  Указатель на целевой bignum_t.
 * @param[in]  str  C-строка с целым числом.
 * @param[in]  base Основание: 0 для автоопределения или 2..16 явно.
 * @return Статус операции.
 */
bignum_init_from_string_status_t bignum_init_from_string(
    bignum_t *dst,
    const char *str,
    int base);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_INIT_FROM_STRING_H */

/* SPDX-License-Identifier: MIT */
