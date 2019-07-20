/*
 * servo.c
 *
 *  Created on: 2019年2月30日
 *      Author: zengwangfa
 *      Notes:  推进器设备
 */
 #define LOG_TAG    "propeller"
 
 
#include <stdlib.h>
#include <string.h>
#include "sys.h"
#include "propeller.h"
#include <elog.h>
#include <rtthread.h>

#include "flash.h"
#include "RC_Data.h"

#define Propeller_MedValue 1500

extern int16 PowerPercent;

uint8 Propeller_Init_Flag = 0;
PropellerParameter_Type PropellerParameter = {//初始化推进器参数值
		 .PowerMax = 2000,//正向最大值
		 .PowerMed = 1500,//中值
		 .PowerMin = 1000,//反向最小值【反向推力最大】
	
	   .PowerDeadband = 10	//死区值
}; 

ActionType_Enum      Posture_Flag;                     //机器人姿态标志位
PropellerDir_Type    PropellerDir = {1,1,1,1,1,1};     //推进器方向，默认为1
PropellerPower_Type  PropellerPower = {0,0,0,0,0,0,0}; //推进器推力控制器
PropellerError_Type  PropellerError = {0,0,0,0,0,0};   //推进器偏差值


PropellerPower_Type power_test; //调试用的变量

void PWM_Update(PropellerPower_Type* propeller)
{	
		power_test.rightUp     = Propeller_MedValue + propeller->rightUp;
		power_test.leftDown    = Propeller_MedValue + propeller->leftDown;
		power_test.leftUp      = Propeller_MedValue + propeller->leftUp;
		power_test.rightDown   = Propeller_MedValue + propeller->rightDown;
		
		power_test.leftMiddle  = Propeller_MedValue + propeller->leftMiddle;
		power_test.rightMiddle = Propeller_MedValue + propeller->rightMiddle;
	
		if(1 == Propeller_Init_Flag){
				
				TIM_SetCompare1(TIM1,power_test.rightUp);     //右上	 E9	
				TIM_SetCompare2(TIM1,power_test.leftDown);    //左下	 E11
				TIM_SetCompare3(TIM1,power_test.leftUp); 	    //左上   E13
				TIM_SetCompare4(TIM1,power_test.rightDown);   //右下   E14
			
				TIM_SetCompare1(TIM4,power_test.leftMiddle);  //左中   D12
				TIM_SetCompare2(TIM4,power_test.rightMiddle); //右中   D13
		}

}



/*******************************************
* 函 数 名：Propeller_Init
* 功    能：推进器的初始化
* 输入参数：none
* 返 回 值：none
* 注    意：初始化流程：
*           1,接线,上电，哔-哔-哔三声,表示开机正常
*           2,给电调2ms或1ms最高转速信号,哔一声
*           3,给电调1.5ms停转信号,哔一声
*           4,初始化完成，可以开始控制
********************************************/
void Propeller_Init(void)//这边都需要经过限幅在给定  原先为2000->1500
{
		rt_thread_mdelay(1000);//等待外部设备初始化成功
		TIM_SetCompare1(TIM1, 2000);  		//最高转速信号   	水平推进器1号
		TIM_SetCompare2(TIM1, 2000);  		//最高转速信号    水平推进器2号
		TIM_SetCompare3(TIM1, 2000); 		  //最高转速信号    水平推进器3号
		TIM_SetCompare4(TIM1, 2000);  		//最高转速信号    水平推进器4号
	
		TIM_SetCompare1(TIM4, 2000); 	 	//最高转速信号  	垂直推进器1号
		TIM_SetCompare2(TIM4, 2000);	  //最高转速信号  	垂直推进器2号

		//TIM_SetCompare3(TIM4, 1900);		//中值
		//TIM_SetCompare4(TIM4, 1700);		//向上	

		rt_thread_mdelay(2000);  //2s


		TIM_SetCompare1(TIM1, 1500);			//停转信号
		TIM_SetCompare2(TIM1, 1500);			//停转信号
		TIM_SetCompare3(TIM1, 1500);			//停转信号
		TIM_SetCompare4(TIM1, 1500);			//停转信号
	
		TIM_SetCompare1(TIM4, 1500);		  //停转信号
		TIM_SetCompare2(TIM4, 1500);		  //停转信号

		TIM_SetCompare3(TIM4, 1900);		//中值
		//TIM_SetCompare4(TIM4, 1000);		//向下
		rt_thread_mdelay(1000);  //1s
		
	  log_i("Propeller_Init()");
		Propeller_Init_Flag = 1;
}


/*【推进器】 【Stop】MSH方法 */
void Propeller_Stop(void)
{
		memset(&PropellerPower,0,sizeof(PropellerPower));

}
MSH_CMD_EXPORT(Propeller_Stop,ag: Propeller Stop);



/*【推进器】 修改 【正向最大值】MSH方法 */
static int propeller_maxvalue_set(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: propeller_maxvalue_set 1600");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1]) <= 2000){
				PropellerParameter.PowerMax = atoi(argv[1]);
				Flash_Update();
				log_i("Current propeller max_value_set:  %d",PropellerParameter.PowerMax);
		}
		
		else {
				log_e("Error! The value is out of range!");
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(propeller_maxvalue_set,ag: propeller set 1600);


/*【推进器】 修改 【正向最大值】MSH方法 */
static int propeller_medvalue_set(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: propeller_medvalue_set 1500");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1]) <= 1500){
				PropellerParameter.PowerMed = atoi(argv[1]);
				Flash_Update();
				log_i("Current propeller med_value_set:  %d",PropellerParameter.PowerMed);
		}
		
		else {
				log_e("Error! The value is out of range!");
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(propeller_medvalue_set,ag: propeller set 1500);

/*【推进器】 修改 【正向最大值】MSH方法 */
static int propeller_minvalue_set(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: propeller_minvalue_set 1600");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1]) <= 1500){
				PropellerParameter.PowerMin = atoi(argv[1]);
				Flash_Update();
				log_i("Current propeller min_value_set:  %d",PropellerParameter.PowerMin);
		}
		
		else {
				log_e("Error! The value is out of range!");
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(propeller_minvalue_set,ag: propeller set 1200);



/*【推进器】 修改 【方向】MSH方法 */
static int propeller_dir_set(int argc, char **argv) //只能是 -1 or 1
{
    int result = 0;
    if (argc != 7){ //6个推进器
				log_i("Propeller: rightUp      leftDown     leftUp     rightDown     leftMiddle    rightMiddle");   //其标志只能是 1  or  -1 
        log_e("Error! Proper Usage: propeller_dir_set 1 1 1 1 1 1 ");
				result = -RT_ERROR;
        goto _exit;
    }

	
		
		if(abs(atoi(argv[1])) == 1 && abs(atoi(argv[2])) == 1  && abs(atoi(argv[3])) == 1  && \
			 abs(atoi(argv[4])) == 1  && abs(atoi(argv[5])) == 1  && abs(atoi(argv[6])) == 1  ) {
				 
				PropellerDir.rightUp     = atoi(argv[1]);
				PropellerDir.leftDown    = atoi(argv[2]);
				PropellerDir.leftUp      = atoi(argv[3]);
				PropellerDir.rightDown   = atoi(argv[4]);
				PropellerDir.leftMiddle  = atoi(argv[5]);
				PropellerDir.rightMiddle = atoi(argv[6]);
				 
				Flash_Update();
				rt_kprintf("\n");
				log_i("Propeller: rightUp      leftDown     leftUp     rightDown     leftMiddle    rightMiddle");   //其标志只能是 1  or  -1 
				log_i("Propeller:    %d           %d          %d          %d            %d             %d",\
				 atoi(argv[1]),atoi(argv[2]),atoi(argv[3]),atoi(argv[4]),atoi(argv[5]),atoi(argv[6]));
		}
		
		else {
				log_e("Error! Input Error!");
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(propeller_dir_set,propeller <1 1 1 1 1 1>);


/*【推进器】 修改 【动力】MSH方法 */
static int propeller_power_set(int argc, char **argv) //只能是 0~3.0f
{
    int result = 0;
    if (argc != 2){ //6个推进器
        log_e("Error! Proper Usage: propeller_power_set <0~300> % ");
				result = -RT_ERROR;
        goto _exit;
    }
		
	  if( atoi(argv[1]) >=0 && atoi(argv[1]) <=300  ) {
				 
				PowerPercent = atoi(argv[1]); //百分制
				Flash_Update();

				log_i("Propeller_Power: %d %%",PowerPercent);
		}
		
		else {
				log_e("Error! Input Error!");
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(propeller_power_set,propeller_power_set <0~300> %);


