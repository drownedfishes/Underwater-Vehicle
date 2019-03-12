#define LOG_TAG    "led"

#include "init.h"
#include "string.h"
#include "flash.h"
#include "drv_ano.h"
/*---------------------- Constant / Macro Definitions -----------------------*/
//RGB灯引脚号
#define LED_Red 		68 
#define LED_Green 	69
#define LED_Blue 		70

//OV Camera 闪光灯
#define LED_Camera 	141  //高电平点亮

/*----------------------- Variable Declarations -----------------------------*/
/* ALL_init 事件控制块. */
extern struct rt_event init_event;
extern rt_uint8_t VehicleStatus;

Bling_Light Light_1,Light_2,Light_3;
u16 Bling_Mode=0;
/*----------------------- Function Implement --------------------------------*/
void led_thread_entry(void *parameter)
{
	  u8 i=0;/*颜色节拍表> 空   红   绿   蓝   青   粉   黄   白 */
		u8 inputdata[8] = {0x00,0x04,0x02,0x01,0x03,0x05,0x06,0x07};
		
		while(i <= 7){
				system_led_blink(inputdata[i++]);}
		
		LED_OFF(LED_Red);			//初始化为高电平 【熄灭】
		LED_OFF(LED_Green);			
		LED_OFF(LED_Blue);
				
    while (1)
    {			

				Bling_Working(0);
				Save_Or_Reset_PID_Parameter();  //FLASH保存 或者 复位PID参数
				
				//led_blink_task();
				rt_thread_mdelay(5);
    }
}

/* led闪烁任务【系统正常运行指示灯】 */
void led_blink_task(void)
{
		static rt_uint8_t status = 1;
		if(boma_value_get() == 1){
				LED_Turn(LED_Green,status);	//初始化为高电平 【熄灭】
		}
}
	

/* 系统初始化led闪烁状态【显示7种颜色】 -->[颜色节拍表> 空  红  绿  蓝  青  粉  黄  白 ] */
void system_led_blink(rt_uint8_t InputData)
{
    if(InputData & 0x04){	
						LED_ON(LED_Red); }
		else{ 	LED_OFF(LED_Red);}
		
	  if(InputData & 0x02){	
					LED_ON(LED_Green); }
		else{ LED_OFF(LED_Green);}
		
		if(InputData & 0x01){	
					 LED_ON(LED_Blue); }
		else{  LED_OFF(LED_Blue);}
		rt_thread_mdelay(300);//等待系统初始化  完毕，系统稳定后，在读取数据
}


int led_thread_init(void)
{
    rt_thread_t led_tid;
		/*创建动态线程*/
    led_tid = rt_thread_create("led",//线程名称
                    led_thread_entry,				 //线程入口函数【entry】
                    RT_NULL,							   //线程入口函数参数【parameter】
                    2048,										 //线程栈大小，单位是字节【byte】
                    30,										 	 //线程优先级【priority】
                    10);										 //线程的时间片大小【tick】= 100ms

    if (led_tid != RT_NULL){
				rt_pin_mode(LED_Red, 	PIN_MODE_OUTPUT);//设置输出模式	
				rt_pin_mode(LED_Green, PIN_MODE_OUTPUT);	
				rt_pin_mode(LED_Blue, 	PIN_MODE_OUTPUT);	
			
				log_i("LED_Init()");
				rt_event_send(&init_event, LED_EVENT);
				rt_thread_startup(led_tid);
		}
		return 0;
}
INIT_APP_EXPORT(led_thread_init);


/* led on MSH方法 */
static int led_on(int argc, char **argv)
{
    int result = 0;

    if (argc != 2){
        log_e("Error! Proper Usage: led_on r\n Species:r \\ g \\ b \\ c");
				result = -RT_ERROR;
        goto _exit;
    }
		
		switch(*argv[1]){
				case 'r':LED_ON(LED_Red);break;
				case 'g':LED_ON(LED_Green);break;
				case 'b':LED_ON(LED_Blue);break;
				case 'c':LED_OFF(LED_Camera);break;
				default:log_e("Error! Proper Usage: led_on R\n Species:r \\ g \\ b \\ c");break;
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(led_on,ag: led_on r  );


/* led off MSH方法 */
static int led_off(int argc, char **argv)
{
    int result = 0;

    if (argc != 2){
        log_e("Error! Proper Usage: led_off r\n Species:r /g /b /c");
				result = -RT_ERROR;
        goto _exit;
    }
		
		switch(*argv[1]){
			
				case 'r':LED_OFF(LED_Red);break;
				case 'g':LED_OFF(LED_Green);break;
				case 'b':LED_OFF(LED_Blue);break;
				case 'c':LED_ON(LED_Camera);break;
				default:log_e("Error! Proper Usage: led_off r\n Species:r /g /b /c");break;
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(led_off,ag:led_off r);




/***************************************************
函数名: void Bling_Set(Bling_Light *Light,
uint32_t Continue_time,//持续时间
uint16_t Period,//周期100ms~1000ms
float Percent,//0~100%
uint16_t  Cnt,
GPIO_TypeDef* Port,
uint16_t Pin
,uint8_t Flag)
说明:	状态指示灯设置函数
入口:	时间、周期、占空比、端口等
出口:	无
备注:	程序初始化后、始终运行
****************************************************/
void Bling_Set(Bling_Light *Light,
               u32 Continue_time,//持续时间
               u32 Period,//周期100ms~1000ms
               float Percent,//0~100%
               u32  Cnt,
               u32 Port,
               u32 Pin,
               u8 Flag)
{
		Light->Bling_Contiune_Time=(Continue_time/5);//持续时间
		Light->Bling_Period=Period;//周期
		Light->Bling_Percent=Percent;//占空比
		Light->Port=Port;//端口
		Light->Pin=Pin;//引脚
		Light->Endless_Flag=Flag;//无尽模式
}

/***************************************************
函数名: void Bling_Process(Bling_Light *Light)//闪烁运行线程
说明:	状态指示灯实现
入口:	状态灯结构体     
出口:	无
备注:	程序初始化后、始终运行
****************************************************/
void Bling_Process(Bling_Light *Light)//闪烁运行线程
{
  if(Light->Bling_Contiune_Time>=1) {
			Light->Bling_Contiune_Time--;
	}
  else {rt_pin_write(Light->Pin ,0);}//亮
  if(Light->Bling_Contiune_Time != 0//总时间未清0
			||Light->Endless_Flag==1)//判断无尽模式是否开启
  {
			Light->Bling_Cnt++;
			if(5*Light->Bling_Cnt>=Light->Bling_Period){
					Light->Bling_Cnt=0;//计满清零
			}
			if(5*Light->Bling_Cnt <= Light->Bling_Period * Light->Bling_Percent){	
					rt_pin_write(Light->Pin ,0);   //亮
			}
			else {rt_pin_write(Light->Pin ,1);}//灭
  }
	else {	
			LED_OFF(LED_Red);			//初始化为高电平 【熄灭】		
			LED_OFF(LED_Blue);
	}
}



/***************************************************
函数名: Bling_Working(uint16 bling_mode)
说明:	状态指示灯状态机
入口:	当前模式
出口:	无
备注:	程序初始化后、始终运行
****************************************************/
void Bling_Working(u16 bling_mode)
{
		if(bling_mode==0)
		{
				Bling_Process(&Light_1);
				Bling_Process(&Light_2);
				Bling_Process(&Light_3);
		}
		else if(bling_mode==1)//加速度计6面校准模式
		{
		 
		}
		else if(bling_mode==2)//磁力计校准模式
		{
			
		}
		else if(bling_mode==3)//全灭
		{

		}
		 
}




