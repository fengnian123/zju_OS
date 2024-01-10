#include "print.h"
#include "sbi.h"
void puts(char *s) {
    // unimplemented
    for(int i = 0 ; s[i] ; i++){
        sbi_ecall(0x1,0,s[i],0,0,0,0,0);
    }
}

void puti(int x) {
    // unimplemented
    int length;
    int temp = x;
    sbi_ecall(0x1,0,30,0,0,0,0,0);
    for(int i = 0; temp ; i++){
        temp /= 10;
        length += 1;
    }
    char s[length+1];
    for(int i = 0 ; i < length ; i++){
        s[i] = '0' + x % 10;
        x /= 10;
    }
    for(int i = length-1 ; i >= 0 ; i--){
        sbi_ecall(0x1,0,s[i],0,0,0,0,0);
    }
}

void put64(uint64 x){
    int length;
    uint64 temp = x;
    sbi_ecall(0x1,0,30,0,0,0,0,0);
    for(int i = 0; temp ; i++){
        temp /= 2;
        length += 1;
    }
    char s[length+1];
    for(int i = 0 ; i < length ; i++){
        s[i] = '0' + x % 2;
        x /= 2;
    }
    for(int i = length-1 ; i >= 0 ; i--){
        sbi_ecall(0x1,0,s[i],0,0,0,0,0);
    }
}