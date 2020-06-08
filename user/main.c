#include <stdio.h>
#include <reg52.h>

typedef  unsigned char   uint8_t;
typedef  unsigned short  uint16_t;
typedef  unsigned long   uint32_t;

//   MCU:89C52RC        11.0592MHz

#define  SIZE   8			 /* 一定要是8的倍数，因为加密和解密数据的最小单元为2个32位数据，即8个字节 */

uint8_t    enbuffer[SIZE];
uint8_t*   key = (uint8_t*)"1234567891234567";	/* 16字节、128bit的key */



/*************  串口1初始化函数 *****************/
void debug_config(void)
{
		SCON=0x50;  //串口方式1，使能接收
    TMOD|=0x20;  //定时器1工作方式2（8位自动重装初值）
    TMOD&=~0x10;
    TH1=0xfa;   //9600bps
    TL1=0xfa;  
    PCON|=0x80;  //SMOD=1
    TR1=1;
    TI=0;
    RI=0;
    //PS=1;   //提高串口中断优先级
}

char putchar (char txdata)
{
		SBUF = txdata;
    while(!TI);
    TI = 0;       

		return 0;
}


//两个32位数据，32次循环加密
static void tea_encrypt(uint32_t *v,uint32_t *k)   
{  	
		uint32_t v0=v[0], v1=v[1], sum=0, i;           /* set up, i为加密轮数 */
    uint32_t delta=0x9e3779b9;                     /* a key schedule constant,神秘常数δ作为倍数   */
    uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */

    for (i=0; i < 32; i++) 
		{  /* basic cycle start,循环加密 */
        sum += delta;
        v0 += ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
        v1 += ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
    }                                              /* end cycle */
    v[0]=v0; 
		v[1]=v1; 
		printf("encrypt sum is %ul \r\n", sum); 
}  

uint16_t encrypt(uint8_t *src, uint16_t size_src, uint8_t *key)  
{  
    uint8_t a = 0;  
    uint16_t i = 0;  
    uint16_t num = 0;  

    //将明文补足为8字节的倍数  
    a = size_src % 8;  
    if (a != 0)  
    {  
        for (i = 0;i < 8 - a;i++)  
        {  
            src[size_src++] = 0;  
        }  
    }  

    //加密  
    num = size_src / 8;  
    for (i = 0;i < num;i++)  
    {  
        tea_encrypt((uint32_t *)(src + i * 8),(uint32_t *)key);  
    }  

    return size_src; 
}


//解密器的实现
static void tea_decrypt(uint32_t *v,uint32_t *k)   
{  
    uint32_t v0=v[0], v1=v[1];
		uint32_t sum = 0xC6EF3720;
		uint32_t i = 0;  /* set up */
    uint32_t delta=0x9e3779b9;                     /* a key schedule constant */
    uint32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */
    for (i=0; i<32; i++) {                         /* basic cycle start */
        v1 -= ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
        v0 -= ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
        sum -= delta;
    }                                              /* end cycle */
    v[0]=v0; v[1]=v1; 
}  

uint16_t decrypt(uint8_t *src,uint16_t size_src,uint8_t *key)  
{  
    uint16_t i = 0;  
    uint16_t num = 0;  

    //判断长度是否为8的倍数  
    if (size_src % 8 != 0)  
    {  
        return 0;  
    }  

    //解密  
    num = size_src / 8;  
    for (i = 0;i < num;i++)  
    {  
        tea_decrypt((uint32_t *)(src + i * 8),(uint32_t *)key);  
    }  

    return size_src;  
}  


void main(void)
{
		uint16_t i=0;

		for(i=0; i<SIZE; i++)
		{
		  /* 赋测试值 */
			enbuffer[i] = i+0x30;
		}

		debug_config();

		encrypt(enbuffer, SIZE, key);
	  printf("encrypt data is: %s \r\n", enbuffer);

		decrypt(enbuffer, SIZE, key);
		printf("decrypt data is: %s \r\n", enbuffer);
		
		return;
}