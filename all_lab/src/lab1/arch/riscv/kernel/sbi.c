#include "types.h"
#include "sbi.h"
struct sbiret sbi_ecall(int ext, int fid, uint64 arg0,
			            uint64 arg1, uint64 arg2,
			            uint64 arg3, uint64 arg4,
			            uint64 arg5) 
{	
    // OpenSBI 的返回结果      
	long ret_error;
	long ret_value;
	__asm__ volatile (
        "mv a7, %[ext]\n"//将 ext 放入寄存器 a7 
		"mv a6, %[fid]\n"//将 fid 放入寄存器 a6
		//将 arg0 ~ arg5 放入寄存器 a0 ~ a5 中
		"mv a0, %[arg0]\n"
		"mv a1, %[arg1]\n"
		"mv a2, %[arg2]\n"
		"mv a3, %[arg3]\n"
		"mv a4, %[arg4]\n"
		"mv a5, %[arg5]\n"
		"ecall\n" //进入 M 模式
		"mv %[ret_error], a0\n"
		"mv %[ret_value], a1\n"
		: [ret_error] "=r" (ret_error), [ret_value] "=r" (ret_value)
        : [ext] "r" (ext), [fid] "r" (fid), [arg0] "r" (arg0), [arg1] "r" (arg1), 
		[arg2] "r" (arg2), [arg3] "r" (arg3), [arg4] "r" (arg4), [arg5] "r" (arg5)
    );
	//用 sbiret 来接受这两个返回值
	struct sbiret ret;
	ret.error=ret_error;
	ret.value=ret_value;
	return ret;
}
