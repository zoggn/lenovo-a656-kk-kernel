#include <lcm_drv.h>
#ifdef BUILD_LK
#include <platform/disp_drv_platform.h>
#include <platform/mt_gpio.h>
#else
#include <linux/delay.h>
#include <mach/mt_gpio.h>
#endif
#include <cust_gpio_usage.h>
//used to identify float ID PIN status
#define LCD_HW_ID_STATUS_LOW      0
#define LCD_HW_ID_STATUS_HIGH     1
#define LCD_HW_ID_STATUS_FLOAT 0x02
#define LCD_HW_ID_STATUS_ERROR  0x03

extern LCM_DRIVER tianma_otm9605a_lcm_drv;
extern LCM_DRIVER tianma_otm9608a_lcm_drv;
extern LCM_DRIVER truly_nt35516_lcm_drv;

LCM_DRIVER* lcm_driver_list[] = 
{ 
#if defined(OTM9605A_QHD_TIANMA)
	&tianma_otm9605a_lcm_drv,
#endif
#if defined(OTM9608A_QHD_TIANMA)
	&tianma_otm9608a_lcm_drv,
#endif
#if defined(NT35516_QHD_TRULY)
	&truly_nt35516_lcm_drv,
#endif
};

#define LCM_COMPILE_ASSERT(condition) LCM_COMPILE_ASSERT_X(condition, __LINE__)
#define LCM_COMPILE_ASSERT_X(condition, line) LCM_COMPILE_ASSERT_XX(condition, line)
#define LCM_COMPILE_ASSERT_XX(condition, line) char assertion_failed_at_line_##line[(condition)?1:-1]

unsigned int lcm_count = sizeof(lcm_driver_list)/sizeof(LCM_DRIVER*);
//LCM_COMPILE_ASSERT(0 != sizeof(lcm_driver_list)/sizeof(LCM_DRIVER*));
#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  printf(fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif
static unsigned char lcd_id_pins_value = 0xFF;
/******************************************************************************
Function:       which_lcd_module
Description:    get lcd module value
Input:          none
Output:         none
Return:         lcd module value
Others:          none
******************************************************************************/
unsigned char which_lcd_module()
{
    unsigned char lcd_id0, lcd_id1;
    unsigned char lcd_id;
    unsigned int ret = 0;
    if(0xFF != lcd_id_pins_value)
    {
        return lcd_id_pins_value;
    }
    ret = mt_set_gpio_mode(GPIO_DISP_ID0_PIN, GPIO_MODE_00);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_mode fail\n");
    }
    ret = mt_set_gpio_dir(GPIO_DISP_ID0_PIN, GPIO_DIR_IN);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_dir fail\n");
    }
    ret = mt_set_gpio_pull_enable(GPIO_DISP_ID0_PIN, GPIO_PULL_ENABLE);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_pull_enable fail\n");
    }
    ret = mt_set_gpio_mode(GPIO_DISP_ID1_PIN, GPIO_MODE_00);
    if(0 != ret)
    {
        LCD_DEBUG("ID1 mt_set_gpio_mode fail\n");
    }
    ret = mt_set_gpio_dir(GPIO_DISP_ID1_PIN, GPIO_DIR_IN);
    if(0 != ret)
    {
        LCD_DEBUG("ID1 mt_set_gpio_dir fail\n");
    }
    ret = mt_set_gpio_pull_enable(GPIO_DISP_ID1_PIN, GPIO_PULL_ENABLE);
    if(0 != ret)
    {
        LCD_DEBUG("ID1 mt_set_gpio_pull_enable fail\n");
    }
    lcd_id0 = mt_get_gpio_in(GPIO_DISP_ID0_PIN);
    lcd_id1 = mt_get_gpio_in(GPIO_DISP_ID1_PIN);

#ifdef BUILD_LK
    printf("which_lcd_module,lcd_id0:%d\n",lcd_id0);
    printf("which_lcd_module,lcd_id1:%d\n",lcd_id1);
#else
    printk("which_lcd_module,lcd_id0:%d\n",lcd_id0);
    printk("which_lcd_module,lcd_id1:%d\n",lcd_id1);
#endif
    lcd_id =  lcd_id0 | (lcd_id1 << 1);

#ifdef BUILD_LK
    printf("which_lcd_module,lcd_id:%d\n",lcd_id);
#else
    printk("which_lcd_module,lcd_id:%d\n",lcd_id);
#endif
    lcd_id_pins_value = lcd_id;
    return lcd_id;
}
/******************************************************************************
Function:       which_lcd_module_Tmp
Description:    get lcd module value , used for G610-T11 V1 board, only ID0 PIN
Input:          none
Output:         none
Return:         lcd module value
Others:          none
******************************************************************************/
unsigned char which_lcd_module_Tmp()
{
    unsigned char lcd_id0;
    /*use to judge return value*/
    unsigned int ret = 0;
    //only recognise once
    if(0xFF != lcd_id_pins_value)
    {
        return lcd_id_pins_value;
    }
    /*because of gpio no initialization,do it by hand*/
    /*checked return value of function for coverity*/
    ret = mt_set_gpio_mode(GPIO_DISP_ID0_PIN, GPIO_MODE_00);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_mode fail\n");
    }
    ret = mt_set_gpio_dir(GPIO_DISP_ID0_PIN, GPIO_DIR_IN);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_dir fail\n");
    }
    ret = mt_set_gpio_pull_enable(GPIO_DISP_ID0_PIN, GPIO_PULL_ENABLE);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_pull_enable fail\n");
    }

    lcd_id0 = mt_get_gpio_in(GPIO_DISP_ID0_PIN);

#ifdef BUILD_LK
    printf("which_lcd_module,lcd_id0:%d\n",lcd_id0);
#else
    printk("which_lcd_module,lcd_id0:%d\n",lcd_id0);
#endif

    lcd_id_pins_value = lcd_id0;
    return lcd_id0;
}
/******************************************************************************
Function:       which_lcd_module_triple
  Description:    read LCD ID PIN status,could identify three status:high/low/float
  Input:           none
  Output:         none
  Return:         LCD ID1|ID0 value
  Others:         
******************************************************************************/
unsigned char which_lcd_module_triple(void)
{
    unsigned char  high_read0 = 0;
    unsigned char  low_read0 = 0;
    unsigned char  high_read1 = 0;
    unsigned char  low_read1 = 0;
    unsigned char  lcd_id0 = 0;
    unsigned char  lcd_id1 = 0;
    unsigned char  lcd_id = 0;
    //Solve Coverity scan warning : check return value
    unsigned int ret = 0;
    //only recognise once
    if(0xFF != lcd_id_pins_value) 
    {
        return lcd_id_pins_value;
    }
    //Solve Coverity scan warning : check return value
    ret = mt_set_gpio_mode(GPIO_DISP_ID0_PIN, GPIO_MODE_00);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_mode fail\n");
    }
    ret = mt_set_gpio_dir(GPIO_DISP_ID0_PIN, GPIO_DIR_IN);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_dir fail\n");
    }
    ret = mt_set_gpio_pull_enable(GPIO_DISP_ID0_PIN, GPIO_PULL_ENABLE);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_pull_enable fail\n");
    }
    ret = mt_set_gpio_mode(GPIO_DISP_ID1_PIN, GPIO_MODE_00);
    if(0 != ret)
    {
        LCD_DEBUG("ID1 mt_set_gpio_mode fail\n");
    }
    ret = mt_set_gpio_dir(GPIO_DISP_ID1_PIN, GPIO_DIR_IN);
    if(0 != ret)
    {
        LCD_DEBUG("ID1 mt_set_gpio_dir fail\n");
    }
    ret = mt_set_gpio_pull_enable(GPIO_DISP_ID1_PIN, GPIO_PULL_ENABLE);
    if(0 != ret)
    {
        LCD_DEBUG("ID1 mt_set_gpio_pull_enable fail\n");
    }
    //pull down ID0 ID1 PIN    
    ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_DOWN);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_pull_select->Down fail\n");
    }
    ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_DOWN);
    if(0 != ret)
    {
        LCD_DEBUG("ID1 mt_set_gpio_pull_select->Down fail\n");
    }
    //delay 100ms , for discharging capacitance 
    msleep(100);
    //get ID0 ID1 status
    low_read0 = mt_get_gpio_in(GPIO_DISP_ID0_PIN);
    low_read1 = mt_get_gpio_in(GPIO_DISP_ID1_PIN);
    //pull up ID0 ID1 PIN 
    ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_UP);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_pull_select->UP fail\n");
    }
    ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_UP);
    if(0 != ret)
    {
        LCD_DEBUG("ID1 mt_set_gpio_pull_select->UP fail\n");
    }
    //delay 100ms , for charging capacitance 
    msleep(100);
    //get ID0 ID1 status
    high_read0 = mt_get_gpio_in(GPIO_DISP_ID0_PIN);
    high_read1 = mt_get_gpio_in(GPIO_DISP_ID1_PIN);

    if( low_read0 != high_read0 )
    {
        /*float status , pull down ID0 ,to prevent electric leakage*/
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_DOWN);
        if(0 != ret)
        {
            LCD_DEBUG("ID0 mt_set_gpio_pull_select->Down fail\n");
        }
        lcd_id0 = LCD_HW_ID_STATUS_FLOAT;
    }
    else if((LCD_HW_ID_STATUS_LOW == low_read0) && (LCD_HW_ID_STATUS_LOW == high_read0))
    {
        /*low status , pull down ID0 ,to prevent electric leakage*/
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_DOWN);
        if(0 != ret)
        {
            LCD_DEBUG("ID0 mt_set_gpio_pull_select->Down fail\n");
        }
        lcd_id0 = LCD_HW_ID_STATUS_LOW;
    }
    else if((LCD_HW_ID_STATUS_HIGH == low_read0) && (LCD_HW_ID_STATUS_HIGH == high_read0))
    {
        /*high status , pull up ID0 ,to prevent electric leakage*/
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_UP);
        if(0 != ret)
        {
            LCD_DEBUG("ID0 mt_set_gpio_pull_select->UP fail\n");
        }
        lcd_id0 = LCD_HW_ID_STATUS_HIGH;
    }
    else
    {
		#ifdef BUILD_LK
			printf(" Read LCD_id0 error\n");
		#else
			printk(" Read LCD_id0 error\n");
		#endif
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_DISABLE);
        if(0 != ret)
        {
            LCD_DEBUG("ID0 mt_set_gpio_pull_select->Disbale fail\n");
        }
        lcd_id0 = LCD_HW_ID_STATUS_ERROR;
    }


    if( low_read1 != high_read1 )
    {
        /*float status , pull down ID1 ,to prevent electric leakage*/
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_DOWN);
        if(0 != ret)
        {
            LCD_DEBUG("ID1 mt_set_gpio_pull_select->Down fail\n");
        }
        lcd_id1 = LCD_HW_ID_STATUS_FLOAT;
    }
    else if((LCD_HW_ID_STATUS_LOW == low_read1) && (LCD_HW_ID_STATUS_LOW == high_read1))
    {
        /*low status , pull down ID1 ,to prevent electric leakage*/
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_DOWN);
        if(0 != ret)
        {
            LCD_DEBUG("ID1 mt_set_gpio_pull_select->Down fail\n");
        }
        lcd_id1 = LCD_HW_ID_STATUS_LOW;
    }
    else if((LCD_HW_ID_STATUS_HIGH == low_read1) && (LCD_HW_ID_STATUS_HIGH == high_read1))
    {
        /*high status , pull up ID1 ,to prevent electric leakage*/
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_UP);
        if(0 != ret)
        {
            LCD_DEBUG("ID1 mt_set_gpio_pull_select->UP fail\n");
        }
        lcd_id1 = LCD_HW_ID_STATUS_HIGH;
    }
    else
    {
		#ifdef BUILD_LK
			printf(" Read LCD_id1 error\n");
		#else
			printk(" Read LCD_id1 error\n");
		#endif
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_DISABLE);
        if(0 != ret)
        {
            LCD_DEBUG("ID1 mt_set_gpio_pull_select->Disable fail\n");
        }
        lcd_id1 = LCD_HW_ID_STATUS_ERROR;
    }
#ifdef BUILD_LK
    printf("which_lcd_module_triple,lcd_id0:%d\n",lcd_id0);
    printf("which_lcd_module_triple,lcd_id1:%d\n",lcd_id1);
#else
    printk("which_lcd_module_triple,lcd_id0:%d\n",lcd_id0);
    printk("which_lcd_module_triple,lcd_id1:%d\n",lcd_id1);
#endif
    lcd_id =  lcd_id0 | (lcd_id1 << 2);

#ifdef BUILD_LK
    printf("which_lcd_module_triple,lcd_id:%d\n",lcd_id);
#else
    printk("which_lcd_module_triple,lcd_id:%d\n",lcd_id);
#endif

    lcd_id_pins_value = lcd_id;
    return lcd_id;
}
#endif
