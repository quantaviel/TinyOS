#include <klib/fmt.h>
#include <hal/vga.h>

namespace fmt {

    namespace conv {
    
    static char digit[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static void reverse(char *buffer, int len);
    static unsigned divmod64_32(unsigned long long *num, unsigned div);

    int to_string(int num, char *buffer, int base);
    int to_string(unsigned num, char *buffer, int base);
    int to_string(long long num, char *buffer, int base);
    int to_string(unsigned long long num, char *buffer, int base);

    int to_string(int num, char *buffer, int base) {
        if (num >= 0) return to_string((unsigned)num, buffer, base);
        *(buffer++) = '-';
        return to_string((unsigned)(-num), buffer, base) + 1;
    }

    int to_string(unsigned num, char *buffer, int base) {
        if (num == 0) {
            *(buffer++) = '0';
            return 1;
        }
        int length = 0;
        while (num > 0) {
            buffer[length++] = digit[num % base];
            num /= base;
        }
        return length;
    }

    int to_string(long long num, char *buffer, int base) {
        if (num >= 0) return to_string((unsigned long long)num, buffer, base);
        *(buffer++) = '-';
        return to_string((unsigned long long)(-num), buffer, base) + 1;
    }

    int to_string(unsigned long long num, char *buffer, int base) {
        if (num == 0) {
            *(buffer++) = '0';
            return 1;
        }
        int length = 0;
        while (num > 0) {
            buffer[length++] = digit[divmod64_32(&num, base)];
        }
        return length;
    }

    int to_string(long num, char *buffer, int base) {
        if (sizeof(long) == 4) return to_string((int)(num), buffer, base);
        else return to_string((long long)(num), buffer, base);
    }

    int to_string(unsigned long num, char *buffer, int base) {
        if (sizeof(long) == 4) return to_string((unsigned)(num), buffer, base);
        else return to_string((unsigned long long)(num), buffer, base);
    }

    void reverse(char *buffer, int length) {
        if (length == 0 || length == 1) return;
        char *i = buffer, *j = buffer + length - 1;
        while (i < j) {
            char t = *i;
            *i = *j;
            *j = t;
            i++;
            j--;
        }
    }

    unsigned divmod64_32(unsigned long long *num, unsigned div) {
        unsigned high = (unsigned)(*num >> 32);
        unsigned low  = (unsigned)(*num & 0xFFFFFFFF);
        unsigned long long res  = 0;
        unsigned rem  = 0;
        const unsigned lim = (unsigned)((1llu << 32) - 1);
        unsigned temp = (lim % div + 1) % div;
        if (high == 0) {
            rem = low % div;
            res = low / div;
        } else {
            unsigned long long rem_temp = *num;
            int curr_len = __builtin_clzll(div) ;
            while (curr_len >= 0) {
                if (rem_temp >= div * (1llu << curr_len)) {
                    rem_temp -= div * 1llu << curr_len;
                    res += 1llu << curr_len;
                }
                curr_len--;
            }
            rem = (unsigned) rem_temp;
        }
        *num = res;
        return rem;
    }

    } // namespace conv


void print_value(char c){ vga::putchar(c); }
void print_value(const char* s){ vga::write(s); }

void print(const char* fmt)    { vga::write(fmt); }
void println(const char* fmt)  { vga::write(fmt); vga::putchar('\n'); }

void format(char* buffer, const char* fmt) {
    while (*fmt) *buffer++ = *fmt++;
    *buffer = '\0';
}

} // namespace fmt