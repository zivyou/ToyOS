#include <printk.h>
#include <terminal.h>

void copy_dec(char *buffer, int32_t *pos, int32_t data){
    if (data == 0) {
        buffer[*pos] = '0';
        (*pos)++;
        return;
    }
    char tmp[32];
    int i;
    for (i=0; i<32; i++) {
        tmp[i] = 0;
    }

    i=0;
    int j = *pos;
    while (data / 10){
        tmp[i++] = (data % 10) + '0';
        data = data / 10;
    }
    
    if (data) {
        tmp[i] = data + '0';
    }
    for (;i >= 0; i--){
        buffer[j++] = tmp[i];
    }
    *pos = j;
};

void copy_hex(char *buffer, int32_t *pos, int32_t data){
    uint32_t udata = (uint32_t)data;  // Use unsigned for correct hex printing

    if (udata == 0) {
        buffer[*pos] = '0';
        (*pos)++;
        return;
    }
    char tmp[32] = {0, };
    for (int k=0; k<32; k++) tmp[k] = 0;
    int i=0; int j=*pos;
    while (udata/16){
        if (udata % 16 < 10){
            tmp[i] = udata % 16 + '0';
        }else if (udata % 16 >= 10){
            tmp[i] = udata % 16 - 10 + 'a';
        } else {
            // do nothing
        }
        i++;
        udata = udata/16;
    }

    if (udata){
        if (udata < 10)
            tmp[i] = udata + '0';
        else
            tmp[i] = udata - 10 + 'a';
        i++;  // Important: increment i after storing the last digit
    }

    // Copy in reverse order
    for (i--; i >= 0; i--){
        buffer[j++] = tmp[i];
    }

    *pos = j;
}

int printk(const char *format, ...){
    va_list args = 0;
    va_start(args, format);
    static char buffer[80*25];
    

    char *c = (char *)format;
    int32_t i=0;
    while (*c){
        if (*c == '%'){
            int arg; char *tmp; char ch;
            if (*(c+1) == 0) break;
            switch(*(c+1)){
                case 'd':
                    arg = va_arg(args, int32_t);
                    copy_dec(buffer, &i, arg);
                    c+=2;
                    break;
                case 's':
                    tmp = va_arg(args, char *);
                    while (*tmp){
                        buffer[i++] = *tmp;
                        tmp++;
                    }
                    c+=2;
                    break;
                case 'c':
                    ch = va_arg(args, char);
                    buffer[i++] = ch;
                    c+=2;
                    break;
                case 'x':
                    arg = va_arg(args, int32_t);
                    copy_hex(buffer, &i, arg);
                    c+=2;
                    break;
                case 0:
                    break;
                default:
                    buffer[i++] = *c;
                    c++;
                    break;
            }
        }else{
            buffer[i++] = *c;
            c++;
        }
    }
    buffer[i] = 0;
    va_end(args);
    terminal_print(buffer);
    return 0;
}