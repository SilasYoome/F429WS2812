#include "ws2812b.h"
#include "Adafruit_NeoPixel.h"
//#include "delay.h"
#include "stm32f4xx_hal.h"
//#include "BottomMotor.h"
#include <math.h>
#include "tim.h"
/*----------------------------------------------*
 * ���w�q                                       *
 *----------------------------------------------*/
#define FLASH_WHEEL_1 					1
#define PI                      3.1415692f
/*TIM+DMA output*/
#define BIT_1                   38u
#define BIT_0                   19u

void WS2812B_Init(void);
void WS2812_set_flash(uint8_t flash,uint16_t angle);
void WS2812_Process(uint8_t model);


/*----------------------------------------------*
 * ������ƭ쫬����                            *
 *----------------------------------------------*/
void WS2812_show(void);

/*----------------------------------------------*
 * �����ܫG                                     *
 *----------------------------------------------*/
uint8_t rBuffer[PIXEL_MAX] = {0};
uint8_t gBuffer[PIXEL_MAX] = {0};
uint8_t bBuffer[PIXEL_MAX] = {0};

/*----------------------------------------------*
 * �Ҷ����ܶq                                   *
 *----------------------------------------------*/
typedef struct
{
    const uint16_t head[3];              //���o�e3��0����dma�P�w
    uint16_t data[24 * PIXEL_MAX];       //�u�������u
    const uint16_t tail;                 //�̦Z�o�e�@��0�A�O��dma�����Z�Apwm��X�C
} frame_buf_ST;
typedef struct
{
	uint16_t data[24 * PIXEL_MAX];
	const uint16_t tail;
} frame2_buf_ST;

frame_buf_ST frame = { .head[0] = 0,
                       .head[1] = 0,
                       .head[2] = 0,
                       .tail    = 0,
                     };

uint8_t     gFlash_Mode     = 2;
uint8_t     f9_state        = 0;
int16_t     Flash1_speed_L, Flash1_speed_R;
										 
/*---------------------------------�������---------------------------------*/
/*****************************************************************************
 �� ? �W  : WS2812B_Init
 �\��y�z  : WS2812��l�ơA�N�Ҧ�led�M�s
            TIM5�MDMA����l�Ʀbmain��ƨ���
 ?�J??  : void
 ?�X??  : �L
 �� �^ ��  :
 ?�Ψ�?  :
 �Q?��?  :
*****************************************************************************/
void WS2812B_Init(void)
{
//    uint32_t i = 0;
//    for(;i<1000;i++)
//    {
//        rainbow(10);
//        WS2812_show();
//        HAL_Delay(10);
//    }
    setAllPixelColor(0, 0, 0);
    WS2812_show();		
}

/*****************************************************************************
 �� ? �W  : WS2812_show
 �\��y�z  : �N���u��z�줺�s�ƲաA�}�q�LDMA�o�e�@�ռ��u
 ?�J??  : void
 ?�X??  : �L
 �� �^ ��  :
 ?�Ψ�?  :
 �Q?��?  :
*****************************************************************************/
void WS2812_show(void)
{
    int8_t i, j,k;
	k = 0;

    for(i = 0; i < PIXEL_MAX; i++)
    {
        for(j = 0; j < 8; j++)
        {
            frame.data[24 * i + j]     = (rBuffer[i] & (0x80 >> j)) ? BIT_1 : BIT_0;
            frame.data[24 * i + j + 8]   = (gBuffer[i] & (0x80 >> j)) ? BIT_1 : BIT_0;
            frame.data[24 * i + j + 16]  = (bBuffer[i] & (0x80 >> j)) ? BIT_1 : BIT_0;
        }
    }
//    for(i = 0; i < PIXEL_MAX; i++)
//    {
//        for(j = 0; j < 8; j++)
//        {
//            frame.data[24 * i + j]      = (rBuffer[PIXEL_MAX-i-1] & (0x80 >> j)) ? BIT_1 : BIT_0;
//            frame.data[24 * i + j + 8]  = (gBuffer[PIXEL_MAX-i-1] & (0x80 >> j)) ? BIT_1 : BIT_0;
//            frame.data[24 * i + j + 16] = (bBuffer[PIXEL_MAX-i-1] & (0x80 >> j)) ? BIT_1 : BIT_0;
//        }
//    }
    HAL_TIM_PWM_Start_DMA(&htim8, TIM_CHANNEL_2, (uint32_t *)&frame, 3 + 24 * PIXEL_MAX + 1);
		HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)&frame, 3 + 24 * PIXEL_MAX + 1);
}

/*****************************************************************************
 �� ? �W  : HAL_TIM_PWM_PulseFinishedCallback
 �\��y�z  : PWM�������_�^�ը�ơA�]��hal�w�ۨ��}�S������PWM,�ҥH�n�b�����Z�ۤv����
 ?�J??  : void
 ?�X??  : �L
 �� �^ ��  :
 ?�Ψ�?  :
 �Q?��?  :
*****************************************************************************/
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    HAL_TIM_PWM_Stop(&htim8,TIM_CHANNEL_2);
		HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_1);
	  
}

/*****************************************************************************
 �� ? �W  : sign
 �\��y�z  : �p�G��J�����ƫh��^1�A���t��^-1�A�_�h��^0
 ��J�Ѽ�  : int16_t db
 ��X�Ѽ�  : �L
 �� �^ ��  : 0�A-1,1
 ?�Ψ�?  :
 �Q?��?  :
*****************************************************************************/
int8_t sign(int16_t db)
{
    int8_t result;

    if(db == 0)
        result = 0;
    else  if( db > 0)
        result = 1;
    else
        result = -1;

    return result;
}

/*****************************************************************************
 �� ? �W  : WS2812B_Wheel_1
 �\��y�z  : ���u�~���]�m���k���t�סA����]���O�t��
             �t��?�m��??void WS2812_set_Wheel_1(int16_t speed_L,int16_t speed_R)�F

 ��J�Ѽ�  : void
 ��X�Ѽ�  : �L
 �� �^ ��  :
 ?�Ψ�?  :
 �Q?��?  : WS2812B_Process
*****************************************************************************/
void WS2812B_Wheel_1(void)  //
{

    static int8_t  cnt_l, cnt_r;
    static int8_t  head_l, head_r;
    //static uint8_t  tab[8]={0,0,0,80,150,170,180,255};
    static uint8_t  tab[8] = {0, 0, 0, 0, 0, 0, 0, 255};
    uint32_t color_buf;
    uint16_t Time_L;
    uint16_t Time_R;
    uint8_t i, n;

    if(Flash1_speed_L == 0)
    {
        Time_L = 0;
    }
    else
    {
        Time_L = 1000 / (16 * fabs(Flash1_speed_L));
    }

    if(Flash1_speed_R == 0)
    {
        Time_R = 0;
    }
    else
    {
        Time_R = 1000 / (16 * fabs(Flash1_speed_R));
    }

    if(cnt_l > Time_L)
    {
        cnt_l = 0;
        n = head_l;

        for(i = 0; i < 8; i++)
        {
            if(n > 7) n = 0;

            color_buf = NEO_WHI(tab[i]);
//            SetPixelColor(n,color_buf);
//            SetPixelColor(n+8,color_buf);
            SetPixelColor(n + 16, color_buf);
            SetPixelColor(n + 24, color_buf);
            n++;
        }

        //�P?���
        if(sign(Flash1_speed_L) == -1)
        {
            head_l++;

            if(head_l > 7)
                head_l = 0;
        }
        else
        {
            head_l--;

            if(head_l < 0)
                head_l = 7;
        }

        //   WS2812_show();
    }

    cnt_l++;

    if(cnt_r > Time_R)
    {
        cnt_r = 0;
        n = head_r;

        for(i = 0; i < 8; i++)
        {
            if(n > 7) n = 0;

            color_buf = NEO_WHI(tab[i]);
            SetPixelColor(n, color_buf);
            SetPixelColor(n + 8, color_buf);
//            SetPixelColor(n+16,color_buf);
//            SetPixelColor(n+24,color_buf);
            n++;
        }

        //�P?���
        if(sign(Flash1_speed_R) == 1)
        {
            head_r++;

            if(head_r > 7)
                head_r = 0;
        }
        else
        {
            head_r--;

            if(head_r < 0)
                head_r = 7;
        }
    }

    cnt_r++;
}
/*****************************************************************************
 �� ? �W  : WS2812_flash_1
 �\��y�z  : �m��y���O 0.5s�@��

 ��J�Ѽ�  : void
 ��X�Ѽ�  : �L
 �� �^ ��  :
 ?�Ψ�?  : WS2812_Process
 �Q?��?  :
*****************************************************************************/
void WS2812_flash_1(void)
{
    const  uint32_t FlashPeriod_ms = 500;
    static uint8_t  state          = 0;
    static uint8_t  numl           = 0;
    static uint8_t  numr           = 0;
    static uint32_t next_time      = 0;
    uint32_t scolor;
    uint32_t timestamp = HAL_GetTick();
    
    uint32_t flag = 0;  
    if(next_time < FlashPeriod_ms)
    {
        if((uint64_t)timestamp + HAL_MAX_DELAY -  next_time > 0)
            flag = 1;
    }
    else if(timestamp > next_time)
    {
        flag = 1;
    }
    
    if(flag)// && timestamp - next_time < FlashPeriod_ms*5)
    {
        next_time = timestamp + FlashPeriod_ms;
        numl++;

        if(numl > 15)
            numl = 0;
        numr = 24 - numl;
        if(numr > 15)
            numr -= 16;
        SetAllPixelColor(0);
        switch(state++)
        {
        case 1:
            scolor = FIX_RED;
            break;
        case 2:
            scolor = FIX_ORG;
            break;
        case 3:
            scolor = FIX_YLW;
            break;
        case 4:
            scolor = FIX_GRN;
            break;
        case 5:
            scolor = FIX_CYA;
            break;
        case 6:
            scolor = FIX_BLU;
            break;
        case 7:
            scolor = FIX_PUR;
            state = 1;
            break;
        default:
            state = 1;
            break;
        }
        SetPixelColor(numl, scolor);
        SetPixelColor(16 + numr, scolor);
    }
}
/*****************************************************************************
 �� ? �W  : WS2812_flash_2
 �\��y�z  : ���y���O 0.5s�@��
 ��J�Ѽ�  : void
 ��X�Ѽ�  : �L
 �� �^ ��  :
 �եΨ��  : WS2812_Process
 �Q�ը��  :
*****************************************************************************/
void WS2812_flash_2(void)
{
    const static uint32_t FlashPeriod_ms = 162;
    static uint8_t numl = 0;
    static uint8_t numr = 0;
    static uint32_t next_time = 0;
    uint32_t timestamp = HAL_GetTick();
    
    static uint8_t  loop = 0;
    if(loop == 0) next_time = timestamp; loop = 1;  //�����եΪ�l��

    if(timestamp > next_time)// && timestamp - next_time < FlashPeriod_ms*5)
    {
        next_time = timestamp + FlashPeriod_ms;
        numl++;

        if(numl >= PIXEL_MAX)
            numl = 0;

        SetAllPixelColor(0);
        SetPixelColor(numl, FIX_GRN);
    }
}

/*****************************************************************************
 �� ? �W  : WS2812_flash_3
 �\��y�z  : ���I�l�O
            �H�����覡���ͩI�l�ĪG
 ��J�Ѽ�  : void
 ��X�Ѽ�  : �L
 �� �^ ��  :
 �եΨ��  : WS2812_Process
 �Q�ը��  :
*****************************************************************************/

void WS2812_flash_3(void)
{
    static uint32_t next_time      = 0;
    const  uint32_t FlashPeriod_ms = 1000;      //�I�l�P��
    const  uint8_t  DPI            = 100;       //����v
    const  uint8_t  max            = 100;       //�̤j�G��
    const  uint8_t  min            = 0;         //�̤p�G��
    static uint8_t  cnt            = 0;
    uint8_t  brighten;
    uint32_t timestamp = HAL_GetTick();
    
    static uint8_t  loop = 0;
    if(loop == 0) next_time = timestamp; loop = 1;  //�����եΪ�l��

    if(timestamp > next_time)// && timestamp - next_time < FlashPeriod_ms*5)
    {
        next_time = timestamp + FlashPeriod_ms / DPI;

        if(cnt++ > DPI)cnt = 0;
        brighten = (max + min) / 2 + ((max - min) / 2) * sin(2 * PI * cnt / DPI);
        SetAllPixelColor(NEO_GRN(brighten));

    }
}
/*****************************************************************************
 �� ? �W  : WS2812_flash_4
 �\��y�z  : �C��I�l�O
 ��J�Ѽ�  : void
 ��X�Ѽ�  : �L
 �� �^ ��  :
 �եΨ��  : WS2812_Process
 �Q�ը��  :
*****************************************************************************/
void WS2812_flash_4(void)
{
    static uint32_t next_time      = 0;
    const  uint32_t FlashPeriod_ms = 1000;      //�I�l�P��
    const  uint8_t  DPI            = 100;       //����v
    const  uint8_t  max            = 100;       //�̤j�G��
    const  uint8_t  min            = 0;         //�̤p�G��
    static uint8_t  cnt            = 0;
    uint8_t  brighten;
    uint32_t timestamp = HAL_GetTick();
    
    static uint8_t  loop = 0;
    if(loop == 0) next_time = timestamp; loop = 1;  //�����եΪ�l��

    if(timestamp > next_time)// && timestamp - next_time < FlashPeriod_ms*5)
    {
        next_time = timestamp + FlashPeriod_ms / DPI;

        if(cnt++ > DPI)cnt = 0;
        brighten = (max + min) / 2 + ((max - min) / 2) * sin(2 * PI * cnt / DPI);
        SetAllPixelColor(NEO_CYA(brighten));

    }
}
/*****************************************************************************
 �� ? �W  : WS2812_flash_5
 �\��y�z  : �C��`�G
 ��J�Ѽ�  : void
 ��X�Ѽ�  : �L
 �� �^ ��  :
 �եΨ��  : WS2812_Process
 �Q�ը��  :
*****************************************************************************/
void WS2812_flash_5(void)
{
    SetAllPixelColor(FIX_CYA);
    WS2812_show();
}

/*****************************************************************************
 �� ? �W  : WS2812_flash_6
 �\��y�z  : ����`�G
 �X�J�Ѽ�  : void
 ��X�Ѽ�  : �L
 �� �^ ��  :
 �եΨ��  : WS2812_Process
 �Q�ը��  :
*****************************************************************************/
void WS2812_flash_6(void)
{
    SetAllPixelColor(FIX_YLW);
    WS2812_show();
}

/*****************************************************************************
 �� ? �W  : WS2812_flash_7
 �\��y�z  : ����`�G
 ��J�Ѽ�  : void
 ��X�Ѽ�  : �L
 �� �^ ��  :
 �եΨ��  : WS2812_Process
 �Q�ը��  :
*****************************************************************************/
void WS2812_flash_7(void)
{
    SetAllPixelColor(FIX_RED);
    WS2812_show();
}

/*****************************************************************************
 �� ? �W  : WS2812_flash_8
 �\��y�z  : ����I�l�O
 ��J�Ѽ�  : void
 ��X�Ѽ�  : ?
 �� �^ ��  :
 �եΨ��  : WS2812_Process
 �Q�ը��  :
*****************************************************************************/
void WS2812_flash_8(void)
{
           uint32_t timestamp      = HAL_GetTick();
    static uint32_t next_time      = 0;
    const  uint32_t FlashPeriod_ms = 1000;      //�I�l�P��
    const  uint8_t  DPI            = 100;       //����v
    const  uint8_t  max            = 100;       //�̤j�G��
    const  uint8_t  min            = 0;         //�̤p�G��
    static uint8_t  cnt            = 0;
    uint8_t  brighten;
    
    static uint8_t  loop = 0;
    if(loop == 0) next_time = timestamp; loop = 1;  //�����եΪ�l��

    if((timestamp > next_time))// && (timestamp - next_time < FlashPeriod_ms*5))
    {
        next_time = timestamp + FlashPeriod_ms / DPI;

        if(cnt++ > DPI)cnt = 0;
        brighten = (max + min) / 2 + ((max - min) / 2) * sin(2 * PI * cnt / DPI);
        SetAllPixelColor(NEO_RED(brighten));

    }
}
/*****************************************************************************
 �� �� �W  : WS2812_flash_9
 �\��y�z  : �C�m�ܴ�
 ��J�Ѽ�  : void
 ��X�Ѽ�  : ?
 �� �^ ��  :
 �եΨ��  : WS2812_Process
 �Q�ը��  :
*****************************************************************************/
void WS2812_flash_9(void)
{
    uint32_t timestamp = HAL_GetTick();
    const  static uint32_t FlashPeriod_ms = 1000;
    static uint32_t next_time = 0;
    
    static uint8_t  loop = 0;
    if(loop == 0) next_time = timestamp; loop = 1;  //�����եΪ�l��
    

    if(f9_state == 0)
    {
        next_time = timestamp + FlashPeriod_ms;
        f9_state = 1;
    }
    if((timestamp > next_time))// && (timestamp - next_time < FlashPeriod_ms*5))
    {
        next_time = timestamp + FlashPeriod_ms;
        f9_state++;

        if(f9_state > 7)
            f9_state = 1;
    }

    switch(f9_state)
    {
    case 1:
        SetAllPixelColor(FIX_RED);
        break;

    case 2:
        SetAllPixelColor(FIX_ORG);
        break;

    case 3:
        SetAllPixelColor(FIX_YLW);
        break;

    case 4:
        SetAllPixelColor(FIX_GRN);
        break;

    case 5:
        SetAllPixelColor(FIX_CYA);
        break;

    case 6:
        SetAllPixelColor(FIX_BLU);
        break;

    case 7:
        SetAllPixelColor(FIX_PUR);
        break;

    case 8:
        SetAllPixelColor(FIX_WHI);
        break;

    case 9:
        SetAllPixelColor(FIX_BLK);
        break;

    default:
        f9_state = 1;
        break;
    }
}


/*****************************************************************************
 �� ? �W  : set_micdir
 �\��y�z  : �]�m���ܤ�V��led
             ���ը��
 �ѤJ�Ѽ�  : void
 ��X�Ѽ�  : ?
 �� �^ ��  :
 �եΨ��  :
 �Q�ը��  :
*****************************************************************************/
void set_micdir(uint16_t data)
{
	const static uint16_t space_angle = 360/PIXEL_MAX;
	
//    if(data == 0)
//    {
//        rainbow(10);
//    }
    int8_t dir_led;
    //static uint32_t led_tab[PIXEL_MAX] = {0x00ff00,0x5F0000,0x3F0000,0x2F0000,0x1F0000,0x0F0000,0x080000,0x010000};	//,0x010000
    int8_t led_mab[20];
    uint8_t i,borrow;
    if(data < 360)
    {
		/*
			�b�@�@��18��LED�����p�U�]1-18�^�Aled1�����׬�350-369(09)LED2=10-29,LED3=30-59 ......
			�ҥH��E�Ƥj�_���_10�ɡA���ӤU��led�G
		*/
    dir_led = (359-data) /space_angle;  //����led���ӫG
		borrow = (359-data) % space_angle;	// �D�E��
		if(borrow < 10) 					//�O�_�ɦ�
		{
			if(--dir_led <0) 
				dir_led = PIXEL_MAX-1;
		}
		
		//�w����F�@��
		dir_led++;
		if(dir_led > PIXEL_MAX-1) 
			dir_led = 0;

		SetAllPixelColor(0);        
		SetPixelColor(dir_led,FIX_GRN);
		
//        led_mab[0] = dir_led;
//		//�Ҽ{�L0�A��z�쥿�`��led����
//        for(i=1; i<8; i++)
//        {
//            led_mab[2*i-1] = dir_led + i;
//            led_mab[2*i]   = dir_led - i;
//            if(led_mab[2*i-1] >= PIXEL_MAX ) led_mab[2*i-1] -= PIXEL_MAX;
//            if(led_mab[2*i]   < 0 )  led_mab[2*i] += PIXEL_MAX;
//        }
//        SetAllPixelColor(0);        
//        SetPixelColor(led_mab[0],led_tab[0]);
//        for(i=1; i<8; i++)
//        {
//            SetPixelColor(led_mab[2*i],led_tab[i]);
//            SetPixelColor(led_mab[2*i-1],led_tab[i]);
//        }
        
    }
}



/*****************************************************************************
 �� �� �W  : WS2812_Process
 �\��y�z  : WS2812�D�i�{���,���Ӵ`���եΡA
 ��J�Ѽ�  : void
 ��X�Ѽ�  : �L
 �� �^ ��  :
 �եΨ��  : freertos.c StartLEDTask02()
 �Q�ը��  :
*****************************************************************************/
void WS2812_Process(uint8_t model)
{
    const uint8_t TestMode = 0xf0;
    {
        gFlash_Mode = model;
        switch(gFlash_Mode)
        {
        case 0x01:
            rainbowCycle(10);
            //WS2812_flash_1();     //�m��]���O
            break;

        case 0x02:            
            WS2812_flash_2();       //���]���O
            break;

        case 0x03:
            WS2812_flash_3();       //���I�l�O
            break;

        case 0x04:
            WS2812_flash_4();       //�C��I�l�O
            break;
        case 0x05:
            WS2812_flash_5();       //�C��`�G
            break;
        case 0x06:
            WS2812_flash_6();       //����`�G
            break;
        case 0x07:
            WS2812_flash_7();       //����`�G
            break;
        case 0x08:
            WS2812_flash_8();       //����I�l
            break;
        case 0x09:
            //WS2812_flash_9();     //�m�����
            rainbow(20);
            break;
				case 0x0A:
						rainbowCycle(10);
						break;
        case 0x0B:
            //rainbowCycle(10);
            rainbow(20);
            break;
        default:
            gFlash_Mode = 0X02;
            break;
        }
        WS2812_show();
				HAL_Delay(10);
    }
}
/*****************************************************************************
 �� �� �W  : WS2812_set_flash
 �\��y�z  : �]�mflash�Ҧ�
 ��J�Ѽ�  : uint8_t flash
 ��X�Ѽ�  : �L
 �� �^ ��  :
 �եΨ��  : uusart.c usart_cmd()
 �Q�ը��  :
*****************************************************************************/
void WS2812_set_flash(uint8_t flash,uint16_t angle)
{
#ifdef DEBUG_ERR
    if(flash > 10)
    {
        printf("flash mode order err \r\n");
    }
#endif
    gFlash_Mode = flash;
    if(gFlash_Mode == 1)
    {
        set_micdir(angle);
    }
    if(flash == 9)      //�O��flash9 �C���q���}�l
    {
        f9_state = 0;
    }
}

/*****************************************************************************
 �� �� �W  : WS2812_set_Wheel_1
 �\��y�z  : �b��t�Ҧ��U�A�]�m�t��
 ��J�Ѽ�  : int16_t speed_L
             int16_t speed_R
 ��X�Ѽ�  : �L
 �� �^ ��  :
 �եΨ��  :
 �Q�ը��  :
*****************************************************************************/
void WS2812_set_Wheel_1(int16_t speed_L, int16_t speed_R)
{
    Flash1_speed_L = speed_L;
    Flash1_speed_R = speed_R;
}


void WS2812B_Test(void)
{
    WS2812_show();
    //return;
    //SetAllPixelColor(0xffffff);
    //SetAllPixelColor(0);
    uint8_t cnt = 0;
    uint16_t num = 0;
    while(1)
    {
        //rainbowCycle(10);
        //rainbow(10);
//        while(!cnt--)
//        {
//            cnt = 1;
            set_micdir(num++);
            if(num >= 360)
                num = 0;
//        }
        
        WS2812_show();
        HAL_Delay(10);
        
        
        //theaterChase(NEO_BLU(100),100);
        //theaterChaseRainbow(10);
    }
}