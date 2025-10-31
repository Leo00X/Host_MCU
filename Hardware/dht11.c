#include "dht11.h"
// #include "delay.h"     // [专家修改] 移除对 delay.h 的依赖
#include "ht32.h"
#include "usart.h"       // 保留，用于调试打印

//IO方向设置 (保持不变)
#define DHT11_IO_IN()  {GPIO_DirectionConfig(HT_GPIOC, GPIO_PIN_0, GPIO_DIR_IN);GPIO_InputConfig(HT_GPIOC, GPIO_PIN_0,ENABLE);}
#define DHT11_IO_OUT() {GPIO_DirectionConfig(HT_GPIOC, GPIO_PIN_0, GPIO_DIR_OUT);GPIO_InputConfig(HT_GPIOC, GPIO_PIN_0,DISABLE);}

#define	DHT11_DQ_IN   GPIO_ReadInBit(HT_GPIOC,GPIO_PIN_0)
      
void DHT11_DQ_OUT(u8 x)
{
	if(x)GPIO_SetOutBits(HT_GPIOC,GPIO_PIN_0);
	else GPIO_ClearOutBits(HT_GPIOC,GPIO_PIN_0);
}

//-------------------------------------------------------------------------------------------------
// [专家新增] BFTM1 阻塞式延时
//-------------------------------------------------------------------------------------------------
/**
 * @brief DHT11专用微秒延时 (忙等待 / 阻塞式)
 * @note  此函数使用 BFTM1 的 One-Shot 模式实现，不依赖任何中断。
 * 它可以在关中断(临界区)内安全使用。
 * (需要假定 SystemCoreClock 变量是准确的)
 * @param us: 要延时的微秒数
 * @retval None
 */
static void DHT11_Delay_us(uint32_t us)
{
    volatile uint32_t target_ticks;
    
    // 计算 BFTM1 (时钟源 PCLK, 假设 = SystemCoreClock) 所需的 Ticks
    // HT32F52352 的 PCLK 默认等于 HCLK (SystemCoreClock)
    uint32_t ticks_per_us = SystemCoreClock / 1000000;
    
    // 确保时钟频率不是太低
    if (ticks_per_us == 0) ticks_per_us = 1; 
    
    target_ticks = us * ticks_per_us;
    
    // 确保延时不为0
    if (target_ticks == 0) target_ticks = 1;
    
    // 配置 BFTM1
    BFTM_SetCompare(HT_BFTM1, target_ticks);
    BFTM_SetCounter(HT_BFTM1, 0);
    
    // 启动 BFTM1 并等待
    BFTM_EnaCmd(HT_BFTM1, ENABLE);
    
    // 循环等待，直到 BFTM1 计数到达 Compare 值
    // (BFTM_FLAG_MATCH 在 One-Shot 模式下即代表延时到达)
    while (BFTM_GetFlagStatus(HT_BFTM1) == RESET)
    {
        // 忙等待
    }
    
    // 清除标志位 (One-Shot模式下，计数器自动停止)
    BFTM_ClearFlag(HT_BFTM1);
}

//-------------------------------------------------------------------------------------------------
// [专家修改] 替换所有 delay_ms / delay_us
//-------------------------------------------------------------------------------------------------

//复位DHT11
void DHT11_Rst(void)	   
{                 
	DHT11_IO_OUT(); 	//SET OUTPUT
    DHT11_DQ_OUT(0); 	//拉低DQ
    // delay_ms(20);    	// [专家修改]
    DHT11_Delay_us(20000); 	//拉低至少18ms (使用20ms)
    DHT11_DQ_OUT(1); 	//DQ=1 
	// delay_us(30);     	// [专家修改]
    DHT11_Delay_us(30);     //主机拉高20~40us
}

//等待DHT11的回应
//返回1:未检测到DHT11的存在
//返回0:存在
u8 DHT11_Check(void) 	   
{   
    // [专家注] 此函数在关中断时调用, UsartPrintf若依赖中断则会导致死锁
	// UsartPrintf(USART_DEBUG, "DHT11_Check(void)\r\n"); 
	u8 retry=0;
	DHT11_IO_IN();//SET INPUT	 
    while (DHT11_DQ_IN&&retry<100)//DHT11会拉低40~80us
	{
		retry++;
		// delay_us(1); // [专家修改]
        DHT11_Delay_us(1);
	};	 
	if(retry>=100)return 1;
	else retry=0;
    while (!DHT11_DQ_IN&&retry<100)//DHT11拉低后会再次拉高40~80us
	{
		retry++;
		// delay_us(1); // [专家修改]
        DHT11_Delay_us(1);
	};
	if(retry>=100)return 1;	    
	return 0;
}
//从DHT11读取一个位
//返回值：1/0
u8 DHT11_Read_Bit(void) 			 
{
 	u8 retry=0;
	while(DHT11_DQ_IN&&retry<100)//等待变为低电平
	{
		retry++;
		// delay_us(1); // [专家修改]
        DHT11_Delay_us(1);
	}
	retry=0;
	while(!DHT11_DQ_IN&&retry<100)//等待变高电平
	{
		retry++;
		// delay_us(1); // [专家修改]
        DHT11_Delay_us(1);
	}
	// delay_us(40); // [专家修改]
    DHT11_Delay_us(40); //等待40us
    
	if(DHT11_DQ_IN)return 1;
	else return 0;		   
}
//从DHT11读取一个字节
//返回值：读到的数据
u8 DHT11_Read_Byte(void)    
{        
    u8 i,dat;
    dat=0;
	for (i=0;i<8;i++) 
	{
   		dat<<=1; 
	    dat|=DHT11_Read_Bit(); // 此函数内部延时已被修改
    }						    
    return dat;
}

/**
 * @brief  从DHT11读取一次数据 (兼容DHT11和DHT22)
 * @param  temp_int: [输出] 温度整数部分
 * @param  temp_dec: [输出] 温度小数部分 (DHT11将始终返回0)
 * @param  humi_int: [输出] 湿度整数部分
 * @param  humi_dec: [输出] 湿度小数部分 (DHT11将始终返回0)
 * @retval 0: 正常
 * @retval 1: 读取失败 (Check失败 或 校验和失败)
 */
// [专家修改] 更改函数定义
u8 DHT11_Read_Data(u8 *temp_int, u8 *temp_dec, u8 *humi_int, u8 *humi_dec)
{        
//	UsartPrintf(USART_DEBUG, "DHT11_Read_Data\r\n");
 	u8 buf[5];
	u8 i;
    u8 check_status = 1; // 默认失败
    u8 checksum_ok = 0;  // 默认校验失败

	//==============================================================
	// 1. 进入临界区 (保持不变)
	//==============================================================
    __disable_irq();

	DHT11_Rst();
	check_status = DHT11_Check(); 
	
	if(check_status == 0)
	{
		for(i=0;i<5;i++)
		{
			buf[i]=DHT11_Read_Byte();
		}
	}

	//==============================================================
	// 2. 退出临界区 (保持不变)
	//==============================================================
    __enable_irq();


    // 3. 在中断恢复后，进行数据处理和校验
	if(check_status == 0)
	{
        // [专家修改] 注意：DHT22的校验和计算方式不同，但DHT11的 buf[1] 和 buf[3] 为0
        // 所以 (buf[0]+buf[1]+buf[2]+buf[3]) 刚好等于 (buf[0]+buf[2])
        // 这里的校验逻辑对DHT11仍然是正确的。
        // (如果使用DHT22，且小数不为0，此校验和依然正确)
        u8 checksum = (buf[0]+buf[1]+buf[2]+buf[3]);
        
		if(checksum == buf[4])
		{
            // [专家修改] 为所有指针赋值
			*humi_int = buf[0];
            *humi_dec = buf[1]; // 湿度小数
			*temp_int = buf[2];
            *temp_dec = buf[3]; // 温度小数
            
            checksum_ok = 1; // 校验和也成功
		}
	}
    
	if (checksum_ok)
    {
        return 0; // 0, 正常
    }
    else
    {
        // 如果读取失败，将值清零，防止调用端使用到上一次的脏数据
        *humi_int = 0;
        *humi_dec = 0;
        *temp_int = 0;
        *temp_dec = 0;
        return 1; // 1, 读取失败
    }
}

//-------------------------------------------------------------------------------------------------
// [专家修改] 初始化 BFTM1
//-------------------------------------------------------------------------------------------------

//初始化DHT11的IO口 DQ 同时检测DHT11的存在
//返回1:不存在
//返回0:存在    	 
u8 DHT11_Init(void)
{	 
    u8 check_status;
    
	//使能PC端口和AFIO的时钟
	CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
	CKCUClock.Bit.PC         = 1;
	CKCUClock.Bit.AFIO       = 1;
	CKCU_PeripClockConfig(CKCUClock, ENABLE);

	
    // [--- 新增代码 ---]
    // 使能 BFTM1 的时钟，用于阻塞延时
    CKCU_PeripClockConfig_TypeDef CKCUClock_BFTM = {{0}};
    CKCUClock_BFTM.Bit.BFTM1 = 1;
    CKCU_PeripClockConfig(CKCUClock_BFTM, ENABLE);
    
    // 配置 BFTM1 为 One-Shot 模式 (单脉冲模式)
    BFTM_OneShotModeCmd(HT_BFTM1, ENABLE);
    // [--- 新增结束 ---]

	
	//配置端口功能为GPIO
	AFIO_GPxConfig(GPIO_PC, GPIO_PIN_0, AFIO_FUN_GPIO);

	//配置IO口为输入模式                                                     
	GPIO_DirectionConfig(HT_GPIOC, GPIO_PIN_0, GPIO_DIR_IN);

	//使能输入
	GPIO_InputConfig(HT_GPIOC, GPIO_PIN_0,ENABLE);
			
    // 关中断执行Rst和Check，确保初始化时序正确
    __disable_irq();
	DHT11_Rst();  //复位DHT11
	check_status = DHT11_Check();//等待DHT11的回应
    __enable_irq();
    
    return check_status;
}

