#include "sys.h"
#include "Data_Base.h"
#include "delay.h"
#include "includes.h"
#include "led.h"
#include "key.h"
#include "Protocal.h"
#include "UART_Interface.h"
#include "transport.h"
  
SerialPakage g_SerialPackRX = {0};
SerialPakage g_SerialPackTX = {0};

extern u8 rec_flag ;
extern Feedback_Msg feedbackMsg;


//�������ȼ�
#define START_TASK_PRIO         3
//�����ջ��С	
#define START_STK_SIZE          100
//������ƿ�
OS_TCB StartTaskTCB;
//�����ջ	
CPU_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *p_arg);


/* Usart task */
#define COMMUNCIATE_TASK_PRIO         4
#define COMMUNCIATE_STK_SIZE          128
OS_TCB COMMUNCIATETaskTCB;
CPU_STK COMMUNCIATE_TASK_STK[COMMUNCIATE_STK_SIZE];
void Communicate_task(void *p_arg);
void Data_Process(SerialPakage *SerialPack);

/* Manual task */
#define MANUAL_TASK_PRIO        4
#define MANUAL_STK_SIZE         256
OS_TCB MANUALTaskTCB;
CPU_STK MANUAL_TASK_STK[MANUAL_STK_SIZE];
void Manual_task(void *p_arg);


int main(void)
{
    
    OS_ERR err;
    CPU_SR_ALLOC();
    
    Write_Through();                //͸д
    Cache_Enable();                 //��L1-Cache
    HAL_Init();				        //��ʼ��HAL��
    Stm32_Clock_Init(432,25,2,9);   //����ʱ��,216Mhz 
    delay_init(216);                //��ʱ��ʼ��

    OSInit(&err);		            //��ʼ��UCOSIII
    OS_CRITICAL_ENTER();            //�����ٽ��� 
    
    //������ʼ����
    OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//������ƿ�
                (CPU_CHAR	* )"start task", 		//��������
                 (OS_TASK_PTR )start_task, 			//������
                 (void		* )0,					//���ݸ��������Ĳ���
                 (OS_PRIO	  )START_TASK_PRIO,     //�������ȼ�
                 (CPU_STK   * )&START_TASK_STK[0],	//�����ջ����ַ
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//�����ջ�����λ
                 (CPU_STK_SIZE)START_STK_SIZE,		//�����ջ��С
                 (OS_MSG_QTY  )0,					//�����ڲ���Ϣ�����ܹ����յ������Ϣ��Ŀ,Ϊ0ʱ��ֹ������Ϣ
                 (OS_TICK	  )0,					//��ʹ��ʱ��Ƭ��תʱ��ʱ��Ƭ���ȣ�Ϊ0ʱΪĬ�ϳ��ȣ�
                 (void   	* )0,					//�û�����Ĵ洢��
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR|OS_OPT_TASK_SAVE_FP, //����ѡ��,Ϊ�˱���������������񶼱��渡��Ĵ�����ֵ
                 (OS_ERR 	* )&err);				//��Ÿú�������ʱ�ķ���ֵ
    OS_CRITICAL_EXIT();	//�˳��ٽ���	 
    OSStart(&err);      //����UCOSIII
    while(1)
    {
    } 
}

//��ʼ������
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;

	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//ͳ������                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//���ʹ���˲����жϹر�ʱ��
    CPU_IntDisMeasMaxCurReset();	
#endif

#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //��ʹ��ʱ��Ƭ��ת��ʱ��
	 //ʹ��ʱ��Ƭ��ת���ȹ���,����Ĭ�ϵ�ʱ��Ƭ����s
	OSSchedRoundRobinCfg(DEF_ENABLED,10,&err);  
#endif		
	
	OS_CRITICAL_ENTER();	//�����ٽ���
         
    /* Usart task */
    OSTaskCreate((OS_TCB 	* )&COMMUNCIATETaskTCB,
                 (CPU_CHAR	* )"Communicate_task", 
                 (OS_TASK_PTR )Communicate_task,
                 (void		* )0,
                 (OS_PRIO	  )COMMUNCIATE_TASK_PRIO,     
                 (CPU_STK   * )&COMMUNCIATE_TASK_STK[0],
                 (CPU_STK_SIZE)COMMUNCIATE_STK_SIZE/10,
                 (CPU_STK_SIZE)COMMUNCIATE_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK	  )0,
                 (void   	* )0,
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR|OS_OPT_TASK_SAVE_FP, 
                 (OS_ERR 	* )&err);
                 
    /* Manual task */
    OSTaskCreate((OS_TCB 	* )&MANUALTaskTCB,
                 (CPU_CHAR	* )" Manual_task", 
                 (OS_TASK_PTR )Manual_task,
                 (void		* )0,
                 (OS_PRIO	  )MANUAL_TASK_PRIO,     
                 (CPU_STK   * )&MANUAL_TASK_STK[0],
                 (CPU_STK_SIZE)MANUAL_STK_SIZE/10,
                 (CPU_STK_SIZE)MANUAL_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK	  )0,
                 (void   	* )0,
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR|OS_OPT_TASK_SAVE_FP, 
                 (OS_ERR 	* )&err);

    OS_CRITICAL_EXIT();//�����ٽ��� 
    OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);//����ʼ����
}

void Communicate_task(void *p_arg)
{
    OS_ERR err = OS_ERR_NONE;
    //UNSED(p_arg);
    LED_Init();
    UART_Init(UART_DEV1, 115200u, UART_WORDLENGTH_8B, UART_STOPBITS_1, UART_PARITY_NONE);
    //UART_DMA_init(&UART_Handler[UART_DEV1]);
    
    //printf("\r\n wait IPC...... \r\n");
    // wait for serial communication to be established successfully(IPC running)
   // OSTimeDlyHMSM(0, 0, 3, 0, OS_OPT_TIME_DLY, &err);
    //printf("\r\n wait IPC...... \r\n");
    
    while(1) 
    {
        //DMA_USART_Transmit(&UART_Handler[UART_DEV1],);
        //OSTimeDly(10u, OS_OPT_TIME_PERIODIC, &err);
        if(rec_flag)
        {
            /*
            printf("\r\n----- Head: -----\r\n\r\n"); 
            printf("moduleId: %X\r\n",g_SerialPackRX.head_.moduleId);
            printf("dataId: %X\r\n",g_SerialPackRX.head_.dataId);
            printf("dataLen: %X\r\n",g_SerialPackRX.head_.dataLen);
            
            printf("\r\n----- Data: -----\r\n\r\n");  
            for(t=0; t<g_SerialPackRX.head_.dataLen; ++t)
            {
                printf("check:%X",g_SerialPackRX.byData_[t]);
            }
            
            printf("\r\n----- Crc: -----\r\n\r\n");
            printf("check: %X\r\n",g_SerialPackRX.check_); 
            */
            
            
            HAL_UART_Transmit(&UART_Handler[UART_DEV1],(uint8_t *)&g_SerialPackRX.head_.dataLen,1,1000);  // send  dataLen
            HAL_UART_Transmit(&UART_Handler[UART_DEV1],(uint8_t *)&g_SerialPackRX,g_SerialPackRX.head_.dataLen+8,1000); //  send data
//            HAL_UART_Transmit(&UART_Handler[UART_DEV1],(uint8_t *)&g_SerialPackRX.check_,2,1000);       //send  crc
            while(__HAL_UART_GET_FLAG(&UART_Handler[UART_DEV1],UART_FLAG_TC)!=SET);               //wait  untill send end
            
            memset(&g_SerialPackRX, 0 , sizeof(SerialPakage));     //clear data
            rec_flag = 0;
        }
        OSTimeDlyHMSM(0,0,0,20,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ200ms
        //Data_Process(&SerialPackRX);
    }
}

void Manual_task(void *p_arg)
{
    OS_ERR err = OS_ERR_NONE;
    u8 key = 0;
    
    KEY_Init();
    while(1)
    {
        OSTimeDly(10u, OS_OPT_TIME_PERIODIC, &err);
        key = KEY_Scan(0);
        switch(key)
        {
            /***feedback test***/
            case KEY0_PRES: 
                //printf("0x03 0x9c 0x50 0x10 0x08 0x00 0x00 0x00 0x80 0x3f 0x00 0x00 0x00 0x00 0x50 0x00");
                feedbackMsg.now_speed = 0.1;
                feedbackMsg.now_angle = 0.0;

                if(feedMsgPack(feedbackMsg))
                {
                    //  send data
                    HAL_UART_Transmit(&UART_Handler[UART_DEV1],(uint8_t *)&g_SerialPackTX, 
                                  HEAD_BYTESIZE+g_SerialPackTX.head_.dataLen,1000);
                    HAL_UART_Transmit(&UART_Handler[UART_DEV1],(uint8_t *)&g_SerialPackTX.check_, 
                                  CRC_BYTESIZE,1000);            
                while(__HAL_UART_GET_FLAG(&UART_Handler[UART_DEV1],UART_FLAG_TC)!=SET);    //wait  untill send end
                    
                }
                
                break;
            
            case KEY1_PRES: printf("\r\n 1 \r\n");
                break;
            case KEY2_PRES: printf("\r\n 2 \r\n");
                break;
            case WKUP_PRES: printf("\r\n 3 \r\n");
                break;  
        }
    }
}


void Data_Process(SerialPakage *SerialPackRX)
{
    ASSERT_NULL_VOID(SerialPackRX);
        
    switch(SerialPackRX->head_.dataId)        
    {
        case CMD_IPC_COMMOND:
            LED0(0);
            //memcpy(&g_APP_USER_Input.control_msg,SerialPack->byData_,SerialPack->head_.dataLen);
            break; 
        
        case DEBUG_QT_COMMOND:
            LED0(1);
            break;
        
        //default:
        //    printf("\r\nInvalid Command !!!\r\n");
    }
    SerialPackRX = NULL;
}
