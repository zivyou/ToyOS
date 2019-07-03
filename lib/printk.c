#include <printk.h>

void copy_dec(char *buffer, int *pos, int data){
    char tmp[14];
    int i=0;
    for (i=0; i<14; i++)
        tmp[i] = 0;
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

int printk(const char *format, ...){
    va_list args = 0;
    va_start(args, format);
    static char buffer[80*50];
    

    char *c = format;
    int i=0;
    while (*c){
        if (*c == '%'){
            int arg; char *tmp; char ch;
            switch(*(c+1)){
                case 'd':
                    arg = va_arg(args, int);
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