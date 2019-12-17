#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include <locale.h>

const int byte_size = 8; // колличество бит в байте

/*
 * подсчет количества контрольных бит
*/
long countKbits(long length)
{
    int sum = 0;

    for (int i = 0; length > 0; i++)
    {
        sum++;
        length -= pow(2,i) - 1;
    }

    return sum;
}

/*
 * вывод данных в двоичном представлении
*/
void coutt(char const* const byte, int const size)
{
    unsigned char mask = 0x80;
    char zn = 0;
    int count = 0;

    for (int j = 0; j < size; j++)
    {
        if (count == 4)
        {
            count = 0;
            printf("\n");
        }
        count++;
        for (int i = 0; i < 8; i++)
        {
            zn = byte[j] & mask >> i;
            if (zn!=0)
            {
                printf("%c", '1');
            }
            else
            {
                printf("%c", '0');
            }
        }
        printf(" ");
    }
    printf("\n");
}

/*
 * Смещение массива вправо
*/
char *const setOffset(char *const message, int const offset)
{
    char* result = NULL;
    int size = 0;

    if (offset < 0 || offset > 9)
    {
        result = message;
        goto exit;
    }

    size = strlen(message);
    result = (char*)malloc(size + 1);

    if (!result)
    {
        result = message;
        goto exit;
    }

    memset(result, 0, size + 1);
    for (int i = 0; i < size; ++i)
    {
        result[i] |= (unsigned char)message[i] >> offset;
        result[i + 1] = (unsigned char)message[i] << (byte_size - offset);
    }

exit:
    if (result != message)
    {
        free(message);
    }
    return result;
}

/*
 * Получение синдрома для заданной последовательности бит
*/
int getSyndrome(char* message, int part, int begin)
{
    unsigned char mask = 0x80;
    int sum = 0;
    int temp = 0;
    int syndrome = 0;

    int kbits = countKbits(part);
    part += kbits;

    // установка контрольных бит
    for (int i = 0; i < kbits; i++)
    {
        sum = 0;
        temp = pow(2, i) - 1;  // позиция контрольного бита
        for (int j = temp; j < part; j += (i + 1) * 2)
        {
            for (int k = 0; k <= i; k++) // подгруппа контролируемых битов
            {
                if (1)//j+k!=temp) // кроме первого контролируемого бита
                {
                    if (message[(j + k + begin) / byte_size]
                      & (mask >> ((j + k + begin) % byte_size)) != 0
                    ){
                        sum++;  // подсчет контрольной суммы
                    }
                }
            }
        }
        if (sum % 2 != 0)  // значение контр.бита - инвертированная кольцевая сумма
        {
            syndrome += i + 1;
        }
    }
    return syndrome;
}

/*
 * кодирование сообщения длиной size бит,
 * память: 61 + size байт
*/
char* hemming(char* part, int size, int pos_p)
{
    unsigned char mask = 0x80; // маска для чтения "10000000"
    int temp = 0;
    int kbits = countKbits(size); // количество контрольных бит
    int enc_size = size + kbits;
    int part_size = (size + kbits + 1) / byte_size;
        part_size += ((size + kbits + 1) % byte_size > 0 ? 1 : 0); // размер закодированной части
    char buf = 0, *result = (char*)malloc(part_size), *buff = NULL;
    int r_offset = 0; // pos_p - позиция в part, r_offset - смещение для вставки в result

    memset(result, 0, part_size);

    // Заполнение массива исходными данными
    for (int i = 0; i < kbits; i++)
    {
        temp = pow(2,i); // позиция контрольного бита
        /* от текущего до следующего контрольного бита */
        for (int pos_r = temp; pos_r < temp * 2 - 1 && pos_r < enc_size; ++pos_r)
        {
            buf = part[pos_p / byte_size] & (mask >> pos_p % byte_size); // получение бита из part
            r_offset = pos_r % byte_size - pos_p % byte_size; // смещение этого бита
            result[pos_r / byte_size] |= (r_offset >= 0) ? buf >> r_offset : buf << -r_offset; // запись бита в result
            pos_p++;
        }
    }

    // установка контрольных бит
    for (int i = 0; i < kbits; i++)
    {
        pos_p = 0;  // pos_p - используется дважды, чтобы не использовать еще одну переменную
        temp = pow(2,i) - 1; // позиция контрольного бита
        for (int j = temp; j < enc_size; j += (i + 1) * 2)
        {
            for (int k = 0; k <= i; k++) // подгруппа контролируемых битов
            {
                if (j + k != temp) // кроме первого контролируемого бита
                {
                    if ((result[(j + k) / byte_size] & (mask >> (j + k) % byte_size)) != 0)
                    {
                        pos_p++; // подсчет контрольной суммы
                    }
                }
            }
        }
        if (pos_p % 2 != 0) // значение контр.бита - инвертированная кольцевая сумма
        {
            result[temp / byte_size] |= mask >> temp % byte_size;
        }
    }
    // установка бита четности
    result = setOffset(result, 1); // единичное смещение
    pos_p = 0; // повторное использование переменной
    for (int i = 1; i <= enc_size; ++i)
    {
        if ((result[i / byte_size] & (mask >> (i % byte_size))) != 0) pos_p++; // подсчет контрольной суммы
    }
    if (pos_p % 2 != 0)
    {
        result[0] |= mask; // установка контрольного бита
    }

    return result;
}



// кодрование по хеммингу
char* setCode(char* message, int part)
{
    int mes_p=0, res_p=0;
    char* temp = NULL, *buf = NULL;
    int mes_size = strlen(message);
    int mes_bits = mes_size * byte_size;
    int count_parts = (mes_bits % part > 0) ? mes_bits / part + 1 : mes_bits / part;
    int kbits = countKbits(part);
    int part_bytes = ((part + kbits + 1) % byte_size > 0) ? (part + kbits + 1) / byte_size + 1 : (part + kbits + 1)/byte_size;
    int res_size = ((kbits + 1) * count_parts % byte_size > 0) ? (kbits + 1) * count_parts / byte_size + 1: (kbits + 1) * count_parts / byte_size;
    res_size += mes_size;
    char* result = (char*)malloc(res_size);

    if (!result)
    {
        goto exit;
    }

    memset(result, 0, res_size);

    for (int i = 0; i < count_parts; ++i)
    {
        temp = hemming(message, part, mes_p);
        temp = setOffset(temp, res_p % byte_size);
        for (int j = 0; j <= part_bytes; ++j)
        {
            result[res_p / byte_size + j] |= temp[j];
        }
        mes_p += part;
        res_p += part + kbits + 1;
    }

exit:
    return result;
}


// декодирование кода
char* getMessage(char* code, int part){
    unsigned char mask = 0x80;
    unsigned char temp;
    int syndrome, kbits = countKbits(part);
    int part_size = part+1+kbits;
    int count_parts = strlen(code)*byte_size/(part+1+kbits);
    int count = 0, pos_c = 0, pos_r=0, r_offset;

    int res_size = (count_parts * part % byte_size > 0)
        ? count_parts * part / byte_size + 1
        : count_parts * part / byte_size;
    char* result = (char*)malloc(res_size);

    for (int i = 0; i < res_size; i++)
    {
        result[i] = 0;
    }
    for (int j = 0; j < count_parts; ++j)
    {
        count = 0;
        for (int i = 0; i < part_size; ++i)
        {
            if ((code[(pos_c+i)/byte_size] & (mask >> ((pos_c+i)%byte_size))) != 0) count++; // подсчет контрольной суммы
        }
        if (count % 2 == 0) temp = 1; // проверка бита четности
            else temp = 0;

        syndrome = getSyndrome(code, part, pos_c + 1);
        if (syndrome != 0 && temp == 0) // одна ошибка
        {
            code[(pos_c + syndrome + 1) / byte_size] ^= (mask >> ((pos_c + syndrome + 1) % byte_size)); // коррекция
        }
        if (syndrome != 0 && temp == 1 || syndrome == 0 && temp == 0) // две ошибки
        {
            free(result);
            return NULL;
        }
        pos_c+=part_size;
    }
    pos_c = 0;

    // формирование сообщения
    for (int i = 0; i < count_parts; ++i)
    {
        count = 1;
        for (int j = 1; j < part_size; ++j)
        {
            if (j == count) count *= 2;
            else
            {
                r_offset = pos_r % byte_size - (pos_c + j) % byte_size; // смещение этого бита
                temp = code[(pos_c + j) / byte_size] & mask >> (pos_c + j) % byte_size;
                result[pos_r / byte_size] |= (r_offset >= 0) ? temp >> r_offset : temp << -r_offset;
                pos_r++;
            }
        }
        pos_c += part_size;
    }
    return result;
}


int main(int argc, char** argv)
{
    int rc = 0;
    char* ans = NULL;
    char* temp = NULL;
    unsigned char t = 0;

    if (argc < 2)
    {
        rc = 1;
        printf("Usage: ./hemm <string_for_code>\n");
        goto exit;
    }

    ans = setCode(argv[1], strlen(argv[1])); // кодирование
    t = 0x80 >> 5;
    ans[0] ^= t;  // добавление ошибки
    ans[3] ^= t >> 1;  // добавление ошибки
    temp = getMessage(ans, 8);  // декодирование

    printf("%s\n", temp);

exit:
    free(ans);
    free(temp);
    return 0;
}

