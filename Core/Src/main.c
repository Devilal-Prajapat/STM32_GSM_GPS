/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <string.h>
#include "gps.h"
#include "iwdg.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#define DEBUG
#ifdef DEBUG
#define DBG_MSG(...)   printf(__VA_ARGS__)
#else
#define DBG_MSG(...)
#endif
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define MAJOR    0
#define MINOR    1
#define TEST_URL    0
#define HTTP_POST_WAIT_TIME     60000  // 90 seconds
#define GSM_RX_BUFFER_SIZE       1024

// uncomment required sim  and comment other
//#define VODA_IDEA
#define AIRTEL

#if defined(VODA_IDEA)
#define SIM_APN       "internet"
#elif  defined(AIRTEL)
#define SIM_APN       "airtelgprs.com"
#else
#define SIM_APN       "internet"
#endif

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t gps_rx_buf[1024];
char at_tx_cmd_buff[256];
uint8_t GPS_RxBuffer[256];
volatile uint8_t gsm_rx_byte;
volatile uint8_t gps_rx_byte;
uint8_t gps_byte_count;
uint8_t sim800_send_data = 1;
uint8_t sim800_check_response = 0;
uint8_t error_count = 0;
uint8_t gps_recv_complete_flag, gps_string_f;
uint8_t gsm_rx_buf[GSM_RX_BUFFER_SIZE];
static uint16_t gsm_rx_byte_count = 0;
char payload[256];
uint8_t payload_len;
uint32_t http_post_max_time = HTTP_POST_WAIT_TIME;
char gpgga_gprmc[] = "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n";

#if TEST_URL
char http_url[] = "AT+HTTPPARA=URL,http://eb83-27-57-100-36.ngrok.io/gps/gps.php\r\n";
char http_get_url[] = "AT+HTTPPARA=URL,http://361f-27-58-27-182.ngrok.io/gps/gps.php?data=devilal\r\n";
#else
char http_get_url[] = "";
//char http_url[] = "AT+HTTPPARA=URL,http://205.147.101.144:3888/vaishnav_tv/add_location\r\n";
char http_url[] = "AT+HTTPPARA=URL,http://6198-117-99-161-242.ngrok.io/gps/gps.php\r\n";
#endif

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void clear_buffer(uint8_t *buff, uint16_t size);
void copy_buffer(uint8_t *dst, const uint8_t *src, uint16_t size);
void gsm_power_on(void);
void gsm_power_off(void);
void check_sim800_modem_ready(void);
void sim800_init(void);
void sim800_gprs_config(void);
void sim800_http_post(void);
void send_at_cmd(char *pdata);
static void at_receive(void);
static void at_clear_rx_buffer(void);
static void prepare_payload(void);

void enable_interrupt(void);
void disbale_interrupt(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern uint32_t one_sec_delay;
extern uint32_t five_sec_delay;
extern uint32_t sim800_timeout;
extern uint32_t ten_ms_delay; 
extern volatile tim_flag_t tim_flag;

// location variable
extern float latitude;
extern float longitude ;
extern uint8_t is_loc_valid;
char imei[15];
uint8_t is_sms_received = 0;
uint8_t sock_connected = 0;
uint8_t pdp_act = 0;

typedef enum 
{
  ATE0_State,
  AT_CPIN,                     // sim card ready
  AT_GSN,                     // get IMEI number
  AT_CREG_State,              // get registration status
  AT_CGATT_State,             // query  gprs status
  AT_CGATT_SET_State,          // set gprs
  //--------------------------------------------------------------
  AT_SAPBR_CONTYPE_State,     // set CONTYPE in bearer profile
  AT_SAPBR_APN_State,         // set APN in bearer profile
  AT_SAPBR_ACTIVATE_State,    // open gprs context
  AT_SAPBR_IP_State,          // get ip from actiavted bearer
  AT_HTTP_INIT_State,
  AT_HTTPPARA_CID_State,
  AT_HTTPPARA_URL_State,
  AT_HTTPPARA_CONTENT_State,
  AT_HTTP_POST_DATA_LEN_State,
  AT_HTTPDATA_State,
  AT_HTTPACTION_State,          // 1- post, 0- get
  AT_HTTPREAD_State,
  AT_HTTPTERM_State,
  AT_SAPBR_DEACTIVATE_State,   // close gprs context
  //--------------------------------------------------------------
  AT_CPOWD,    // Manual Power Down
  //--------------------------------------------------------------
  AT_D,        // dial phone number used to make call
  AT_H,        // Hang a Call
  AT_CMGF,     // sms text mode
  AT_CMGR,     // for read sms 
  AT_CMGD,     // for Delete SMS
  //--------------------------------------------------------------
  AT_CSTT,     // set APN
  AT_CIICR,    // bring wireless connection
  AT_CIFSR,    // get local ip
  AT_CIPSTART, // Start tcp Connection
  AT_CIPSEND,  // open window_to_send_data
  AT_DATA_TO_SEND, // intermediate state
  AT_CIPCLOSE,
  AT_CIPSHUT,
  //--------------------------------------------------------------  
  AT_GPRS_CONF_DONE,
  AT_HTTP_DONE,
  AT_SMS_DONE,
}SIM800_AT_CMD_State;

SIM800_AT_CMD_State at_state = ATE0_State;

uint8_t sleep_mode_flag = 0;

char *AT_DEBUG[50] = {
  "ATE0_State\r\n",
  "AT_GSN\r\n",                     // get IMEI number
  "AT_CREG_State\r\n",              // get registration status
  "AT_CGATT_State\r\n",             // query  gprs status
  "AT_CGATT_SET_State\r\n",          // set gprs
  //--------------------------------------------------------------
  "AT_SAPBR_CONTYPE_State\r\n",     // set CONTYPE in bearer profile
  "AT_SAPBR_APN_State\r\n",         // set APN in bearer profile
  "AT_SAPBR_ACTIVATE_State\r\n",    // open gprs context
  "AT_SAPBR_IP_State\r\n",          // get ip from actiavted bearer
  "AT_HTTP_INIT_State\r\n",
  "AT_HTTPPARA_CID_State\r\n",
  "AT_HTTPPARA_URL_State\r\n",
  "AT_HTTPPARA_CONTENT_State\r\n",
  "AT_HTTP_POST_DATA_LEN_State\r\n",
  "AT_HTTPDATA_State\r\n",
  "AT_HTTPACTION_State\r\n",          // 1- post, 0- get
  "AT_HTTPREAD_State\r\n",
  "AT_HTTPTERM_State\r\n",
  "AT_SAPBR_DEACTIVATE_State\r\n",   // close gprs context
  //--------------------------------------------------------------
  "AT_CPOWD\r\n",    // Manual Power Down
  //--------------------------------------------------------------
  "AT_D\r\n",        // dial phone number used to make call
  "AT_H\r\n",        // Hang a Call
  "AT_CMGF\r\n",     // sms text mode
  "AT_CMGR\r\n",     // for read sms 
  "AT_CMGD\r\n",     // for Delete SMS
  //--------------------------------------------------------------
  "AT_CSTT\r\n",     // set APN
  "AT_CIICR\r\n",    // bring wireless connection
  "AT_CIFSR\r\n",    // get local ip
  "AT_CIPSTART\r\n", // Start tcp Connection
  "AT_CIPSEND\r\n",  // open window_to_send_data
  "AT_DATA_TO_SEND\r\n", // intermediate state
  "AT_CIPCLOSE\r\n",
  "AT_CIPSHUT\r\n",
  //--------------------------------------------------------------  
  "AT_GPRS_CONF_DONE\r\n",
  "AT_HTTP_DONE\r\n",
  "AT_SMS_DONE\r\n",
};
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  
  /* USER CODE BEGIN 2 */
//  IWDG_Init();
//  HAL_GPIO_WritePin(SUPPLY_GPIO_Port, SUPPLY_Pin, GPIO_PIN_SET);
  HAL_Delay(2000);
  DBG_MSG("soft ver-%d.%d\r\n", MAJOR, MINOR);
  gsm_power_on();
  check_sim800_modem_ready();
  sim800_init();
  sim800_gprs_config();
 
  /* Enable only gpgga and gprmc string  */
  HAL_UART_Transmit(&huart3,(uint8_t *)gpgga_gprmc, strlen(gpgga_gprmc),100);
  HAL_UART_Receive(&huart3,(uint8_t *)gps_rx_buf, 20,1000);
  
  /* fall through some times above cammand not executed */
  HAL_UART_Transmit(&huart3,(uint8_t *)gpgga_gprmc, strlen(gpgga_gprmc),100);
  HAL_UART_Receive(&huart3,(uint8_t *)gps_rx_buf, 20,1000);
  
  HAL_NVIC_SetPriority(USART3_4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART3_4_IRQn);
  __HAL_UART_CLEAR_IT(&huart3,UART_IT_RXNE);
  __HAL_UART_ENABLE_IT(&huart3,UART_IT_RXNE);
  clear_buffer(gps_rx_buf, 256);
  HAL_UART_Receive_IT(&huart3, (uint8_t *)&gps_rx_byte,1);

  HAL_UART_Receive_IT(&huart1, (uint8_t *)&gsm_rx_byte,1);
//  HAL_UART_Transmit(&huart1, (uint8_t *)"AT\r\n", 4, 100);
  /* USER CODE END 2 */
  is_loc_valid = 1;
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if(tim_ten_ms_overflow())
    {
      if(gps_recv_complete_flag == 1)
      {
        gps_recv_complete_flag = 0;
        gps_string_f = 0;
        process_gps_data((char *)GPS_RxBuffer);
        __HAL_UART_CLEAR_IT(&huart3,UART_IT_RXNE);
        __HAL_UART_ENABLE_IT(&huart3,UART_IT_RXNE);
        HAL_NVIC_EnableIRQ(USART3_4_IRQn);
        HAL_UART_Receive_IT(&huart3, (uint8_t *)&gps_rx_byte,1);
      }
    }
    
    if(tim_one_sec_overflow())
    {
      if(strstr((char *)gsm_rx_buf, "+SAPBR 1: DEACT")>0)
      {
        pdp_act = 0;
      }
      
      if(strstr((char *)gsm_rx_buf, "RING")>0)
      {
        // Check Call
        clear_buffer(gsm_rx_buf, GSM_RX_BUFFER_SIZE); 
        gsm_rx_byte_count = 0;
        send_at_cmd("ATH\r\n");
      }     
    }
    
#if 1    
    if(is_loc_valid)
    {
      DBG_MSG("{latitude : %.06f, longitude : %.06f}\r\n",latitude, longitude);
      is_loc_valid = 1;      
      if(pdp_act == 0)
      {
        sim800_gprs_config();
      }
      DBG_MSG("--------------------HTTP POST--------------------");
      sim800_http_post();
      sleep_mode_flag = 1;
    }  
#endif  
    
    if(sleep_mode_flag == 1)
    {
      sleep_mode_flag = 0;
      disbale_interrupt();
      HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
      enable_interrupt();     
    }  
  }
  /* USER CODE END 3 */
}

void disbale_interrupt(void)
{
    HAL_SuspendTick();
    HAL_NVIC_ClearPendingIRQ(USART3_4_IRQn);
    HAL_NVIC_ClearPendingIRQ(USART1_IRQn);
    HAL_NVIC_DisableIRQ(USART3_4_IRQn);
    HAL_NVIC_DisableIRQ(USART1_IRQn);
    HAL_TIM_Base_Stop_IT(&htim6);
    NVIC->ICPR[0] = 0xFFFFFFFF;
    HAL_TIM_Base_Start_IT(&htim7);
}

void enable_interrupt(void)
{
    HAL_ResumeTick();
    HAL_TIM_Base_Stop_IT(&htim7);
    NVIC->ICPR[0] = 0xFFFFFFFF;
    HAL_NVIC_ClearPendingIRQ(USART3_4_IRQn);
    HAL_NVIC_ClearPendingIRQ(USART1_IRQn);
    HAL_NVIC_EnableIRQ(USART3_4_IRQn);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    HAL_TIM_Base_Start_IT(&htim6);
    HAL_UART_Receive_IT(&huart3, (uint8_t *)&gps_rx_byte,1);
    at_clear_rx_buffer();
    at_receive();
}

static void at_clear_rx_buffer(void)
{
  gsm_rx_byte_count= 0;
  memset(gsm_rx_buf, 0x00, sizeof(gsm_rx_buf));
}

static void at_receive(void)
{
  __HAL_UART_CLEAR_IT(&huart1, UART_IT_RXNE);
  HAL_UART_Receive_IT(&huart1, (uint8_t *)&gsm_rx_byte,1);
}

/* USER CODE BEGIN */

void sim800_http_post(void)
{
  uint8_t is_http_post_done = 0;
  uint32_t time_out = ONE_SEC_DELAY;
  at_state = AT_HTTP_INIT_State;
  uint32_t start_time = HAL_GetTick();
  sim800_send_data = 1;
  do{
    if(sim800_send_data)
    {
      DBG_MSG("%s", AT_DEBUG[at_state]);
      at_clear_rx_buffer();
      at_receive();  
      switch(at_state)
      {          
        case AT_HTTP_INIT_State:
          send_at_cmd("AT+HTTPINIT\r\n");
          break;
        
        case AT_HTTPPARA_CID_State:
           send_at_cmd("AT+HTTPPARA=\"CID\",1\r\n");
          break;
        
        case AT_HTTPPARA_URL_State:
           send_at_cmd(http_url);
          break;
        
        case AT_HTTPPARA_CONTENT_State:
          {
            char content_type[] = "AT+HTTPPARA=CONTENT,application/x-www-form-urlencoded\r\n";
            send_at_cmd(content_type);
          }
          break;
          
        case AT_HTTP_POST_DATA_LEN_State:
          {    
            prepare_payload();
            char htpp_data_cmd[100];
            payload_len = payload_len -2;
            sprintf(htpp_data_cmd,"AT+HTTPDATA=%d,10000\r\n",payload_len);
            send_at_cmd(htpp_data_cmd);              
          }
          break;
          
        case AT_HTTPDATA_State:
        {              
           send_at_cmd(payload);
        }
        break;  
        
        case AT_HTTPACTION_State:                        
          send_at_cmd("AT+HTTPACTION=1\r\n");
          time_out = THREE_SEC_DELAY;
          break;  
        
        case AT_HTTPREAD_State:        
           send_at_cmd("AT+HTTPREAD\r\n");
          time_out = ONE_SEC_DELAY;
          break;  
        
        case AT_HTTPTERM_State:        
          send_at_cmd("AT+HTTPTERM\r\n");
          break;  
  
        case AT_CPOWD:        
          send_at_cmd("AT+CPOWD=1\r\n");        
          break;
        
        default:
          break;
      }
      sim800_send_data = 0;      
      tim_flag.sim800_timeout_flag = 0;
      sim800_timeout += time_out;//FIVE_SEC_DELAY;
      sim800_check_response = 1;
    }
    
    if(sim800_timer_overflow())
    {
      DBG_MSG("%s",(char *)gsm_rx_buf);
      if(sim800_check_response)
      {
        sim800_check_response = 0;
        sim800_send_data = 1;
       
        if(strstr((char *)gsm_rx_buf, "+SAPBR 1: DEACT")>0)
        {
          pdp_act = 0;
        }
        
        if(strstr((char *)gsm_rx_buf,"\r\nERROR\r\n")>0)
        {
          error_count++;
          if(error_count > 5)
          {
            HAL_Delay(1000);
            at_state = AT_CPOWD;
           
          }
        }else
        {
          error_count = 0;
        }
        switch(at_state)
        {          
          case AT_HTTP_INIT_State:
            if(strstr((char *)gsm_rx_buf,"\r\nOK\r\n")>0)
            {
              at_state = AT_HTTPPARA_CID_State;;
            }
            else
            {
              at_state = AT_HTTPTERM_State;
            }
            break;
            
          case AT_HTTPPARA_CID_State:
            if(strstr((char *)gsm_rx_buf,"\r\nOK\r\n")>0)
            {
              at_state = AT_HTTPPARA_URL_State;
            }      
            break;  
            
          case AT_HTTPPARA_URL_State:
            if(strstr((char *)gsm_rx_buf,"\r\nOK\r\n")>0)
            {
              at_state = AT_HTTPPARA_CONTENT_State;
            }      
            break;  
            
          case AT_HTTPPARA_CONTENT_State:
            if(strstr((char *)gsm_rx_buf,"\r\nOK\r\n")>0)
            {
              at_state = AT_HTTP_POST_DATA_LEN_State;              
            }      
            break;
            
          case AT_HTTP_POST_DATA_LEN_State:
            if(strstr((char *)gsm_rx_buf,"\r\nDOWNLOAD\r\n")>0)
            {
              at_state = AT_HTTPDATA_State;            
            }      
            break;
            
          case AT_HTTPDATA_State:
            if(strstr((char *)gsm_rx_buf,"\r\nOK\r\n")>0)
            {
              at_state = AT_HTTPACTION_State;            
            }      
            break;
            
          case AT_HTTPACTION_State:
            if(strstr((char *)gsm_rx_buf,"\r\n+HTTPACTION: 1,200,")>0)
            {
              at_state = AT_HTTPREAD_State;            
            }  
            else if(strstr((char *)gsm_rx_buf,"\r\n+HTTPACTION: 1,601,")>0)
            {
              at_state = AT_HTTPTERM_State; 
              
            }else
            {
              sim800_send_data = 0;
              sim800_check_response = 1;
            }
            break;
            
          case AT_HTTPREAD_State:
            if(strstr((char *)gsm_rx_buf,"\r\n+HTTPREAD: ")>0)
            {
              at_state = AT_HTTPTERM_State;
            
            }
//              else if(strstr((char *)gsm_rx_buf,"\r\nOK\r\n")>0)
//            {
//              at_state = AT_HTTPTERM_State;            
//            }            
            break;  
            
          case AT_HTTPTERM_State:
            if(strstr((char *)gsm_rx_buf,"\r\nOK\r\n")>0)
            {
              at_state = AT_HTTP_DONE;  
              is_http_post_done = 1;                
            } 
            else
            {
              at_state = AT_HTTP_DONE;  
              is_http_post_done = 1;  
            }             
            break;            
           
          case AT_CPOWD:
            if(strstr((char *)gsm_rx_buf,"NORMAL POWER DOWN")>0)
            {
              HAL_Delay(2000);
              HAL_NVIC_SystemReset();    
            }      
            break;  
            
          default:
            //at_state=  ATE0_State;
            break;
        }        
      }
      at_clear_rx_buffer();
      at_receive();
    }  
    
   if((HAL_GetTick()-start_time)>http_post_max_time)
   {
     is_http_post_done = 1;
   }
  }while((!is_http_post_done));
}

void sim800_gprs_config(void)
{
  sim800_send_data = 1;
  uint32_t time_out = ONE_SEC_DELAY;
  uint8_t is_gprs_init = 0;
  at_state = AT_CGATT_State;
  uint32_t start_time = HAL_GetTick();
  do{
    if(sim800_send_data)
    {
      DBG_MSG("%s",AT_DEBUG[at_state]);
      at_clear_rx_buffer();
      at_receive();  
      switch(at_state)
      { 
        case AT_CGATT_State:
          send_at_cmd("AT+CGATT?\r\n");
          break;
        
        case AT_CGATT_SET_State:
          send_at_cmd("AT+CGATT=1\r\n");
          break;    
        
        case AT_SAPBR_CONTYPE_State:
          send_at_cmd("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n");
          break;
        
        case AT_SAPBR_APN_State:
          {
            char temp_buff[50];
            sprintf(temp_buff,"AT+SAPBR=3,1,\"APN\","SIM_APN"\r\n"); //@TODO: to be checked
            send_at_cmd(&temp_buff[0]);
          }
          break;  
          
        case AT_SAPBR_DEACTIVATE_State:
          send_at_cmd("AT+SAPBR=0,1\r\n");
          break;    
        
        case AT_SAPBR_ACTIVATE_State:
          send_at_cmd("AT+SAPBR=1,1\r\n");
          break;
        
        case AT_SAPBR_IP_State:
          send_at_cmd("AT+SAPBR=2,1\r\n");
          break;            

        case AT_CPOWD:        
          send_at_cmd("AT+CPOWD=1\r\n");        
          break;
        default:
          break;
      } 
      sim800_send_data = 0;
      tim_flag.sim800_timeout_flag = 0;
      sim800_timeout += time_out;
      sim800_check_response = 1;
    }
    
    if(sim800_timer_overflow())
    {
      DBG_MSG("%s",(char *)gsm_rx_buf);
      if(sim800_check_response)
      {
        sim800_check_response = 0;
        switch(at_state)
        {
					case AT_CGATT_State:
						if(strstr((char *)gsm_rx_buf,"\r\n+CGATT: 1\r\n\r\nOK\r\n")>0)
						{
							at_state = AT_SAPBR_CONTYPE_State;
						}
						else
						{
							at_state = AT_CGATT_SET_State;
						}
						break;
					case AT_CGATT_SET_State:
						if(strstr((char *)gsm_rx_buf,"\r\nOK\r\n")>0)
						{
							at_state = AT_SAPBR_CONTYPE_State;
						}
						break;
            
          case AT_SAPBR_CONTYPE_State:
            if(strstr((char *)gsm_rx_buf,"\r\nOK\r\n")>0)
            {
              at_state = AT_SAPBR_APN_State;;
            }      
            break;
          
          case AT_SAPBR_APN_State:
            if(strstr((char *)gsm_rx_buf,"\r\nOK\r\n")>0)
            {
              at_state = AT_SAPBR_DEACTIVATE_State;
            }      
            break;
            
          case AT_SAPBR_DEACTIVATE_State:
            at_state = AT_SAPBR_ACTIVATE_State;            
            break;

          case AT_SAPBR_ACTIVATE_State:
            if(strstr((char *)gsm_rx_buf,"\r\nOK\r\n")>0)
            {
              at_state = AT_SAPBR_IP_State;  
            }      
            break;
            
          case AT_SAPBR_IP_State:
            if(strstr((char *)gsm_rx_buf,"\r\n+SAPBR: 1,1,\"")>0)
            {
              pdp_act = 1;
              at_state = AT_GPRS_CONF_DONE;
              is_gprs_init = 1;
            }
            else if(strstr((char *)gsm_rx_buf,"\r\n+SAPBR: 1,3,\"")>0)
            {
              at_state = AT_CPOWD;
            }    
            break;            

          case AT_CPOWD:
            if(strstr((char *)gsm_rx_buf,"NORMAL POWER DOWN")>0)
            {
              HAL_Delay(2000);
              HAL_NVIC_SystemReset();    
            }      
            break;            
          default:          
            break;
        }
        sim800_send_data = 1;
      }
      
      if((HAL_GetTick()- start_time)> 60*1000)
      {
        start_time = HAL_GetTick();
        at_state = AT_CPOWD;
      }
    }    
  }while(!is_gprs_init);
}

void sim800_init(void)
{
  uint32_t time_out = ONE_SEC_DELAY;
  uint8_t is_gsm_init = 0;
  at_state = ATE0_State;
  uint32_t start_time = HAL_GetTick(); 
  sim800_send_data = 1;  
  do{
    if(sim800_send_data)
    {
      DBG_MSG("%s", AT_DEBUG[at_state]);
     
      at_clear_rx_buffer();
      at_receive();
      switch(at_state)
      {
        case ATE0_State:
          send_at_cmd("ATE0\r\n"); 
          send_at_cmd("ATE0\r\n");          
          break;
        
         case AT_CPIN:
          send_at_cmd("AT+CPIN?\r\n");
          break;
         
        case AT_GSN:
          send_at_cmd("AT+GSN\r\n");         
          break;
                
        case AT_CMGF:
          send_at_cmd("AT+CMGF=1\r\n");
        break;
        
        case AT_CREG_State:
          send_at_cmd("AT+CREG?\r\n");
          break;
        
        case AT_CPOWD:        
          send_at_cmd("AT+CPOWD=1\r\n");        
          break;  
        
        default:
          break;
      }  
      sim800_send_data = 0;      
      tim_flag.sim800_timeout_flag = 0;
      sim800_timeout += time_out;
      sim800_check_response = 1;
    }
   
    if(sim800_timer_overflow())
    {     
      if(sim800_check_response)
      {
        DBG_MSG("%s",(char *)gsm_rx_buf);
        sim800_check_response = 0;
        switch(at_state)
        {
          case ATE0_State:
            if(strncmp((char *)gsm_rx_buf,"\r\nOK\r\n",6)==0)
            {
              at_state = AT_CPIN;
            }      
            break;
            
          case AT_CPIN:
            if (strstr((char *)gsm_rx_buf, "+CPIN: READY") > 0)
            {
              at_state = AT_GSN;
            }
            break;             
           
          case AT_GSN:
            if(strstr((char *)gsm_rx_buf,"\r\nOK\r\n")>0)
            {
              if(gsm_rx_buf[2] >= 0x30 && gsm_rx_buf[2] <= 0x39)
              {
                strncpy(imei,(char *)&gsm_rx_buf[2],15);
              }
              else
              {
                strncpy(imei,(char *)&gsm_rx_buf[9],15);
              }              
              at_state = AT_CMGF;
            }      
            break;
            
          case AT_CMGF:
            if(strstr((char*)gsm_rx_buf,"\r\nOK\r\n")>0)
            {
              at_state = AT_CREG_State;          
            }
            break;
            
          case AT_CREG_State:
            if(strstr((char *)gsm_rx_buf,"\r\n+CREG: 0,1")>0)
            {
              // home network
              is_gsm_init = 1;              
            }
            else if(strstr((char *)gsm_rx_buf,"\r\n+CREG: 0,5")>0)
            {
              // roaming network
              is_gsm_init = 1;              
            }
            break;
            
          case AT_CPOWD:
            if(strstr((char *)gsm_rx_buf,"NORMAL POWER DOWN")>0)
            {
              HAL_Delay(2000);
              HAL_NVIC_SystemReset();    
            }  
          default:
            break;
        }
        sim800_send_data = 1;
      }
      
      if((HAL_GetTick()- start_time)> 60*1000)
      {
        start_time = HAL_GetTick();
        at_state = AT_CPOWD;
      }   
    } 
  }while(!is_gsm_init);    
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the peripherals clocks
  */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance == huart3.Instance)
  {
    static uint8_t new_line_count = 0;;
    if( new_line_count == 2)
    {      
      __HAL_UART_DISABLE_IT(&huart3, UART_IT_RXNE);
      HAL_NVIC_ClearPendingIRQ(USART3_4_IRQn);
      HAL_NVIC_DisableIRQ(USART3_4_IRQn);
      clear_buffer(GPS_RxBuffer, 256);
      copy_buffer(GPS_RxBuffer,gps_rx_buf,gps_byte_count);
      clear_buffer(gps_rx_buf, gps_byte_count);
      new_line_count = 0;     
      gps_byte_count = 0;
      gps_recv_complete_flag = 1;
    }
    else 
    {
      if((gps_rx_byte == '$') && (gps_byte_count == 0) && (gps_string_f == 0))
      {
        gps_rx_buf[gps_byte_count] = gps_rx_byte;
        gps_byte_count++;
        gps_string_f = 1;        
      }
      else if(gps_string_f == 1)
      {      
        if(gps_rx_byte == '\n')
        {
          new_line_count++;
        }      
        gps_rx_buf[gps_byte_count] = gps_rx_byte;
        gps_byte_count++;
      }
      else
      {
        gps_byte_count = 0;
      }
      HAL_UART_Receive_IT(&huart3, (uint8_t *)&gps_rx_byte, 1);     
    }      
  }

  if(huart->Instance == USART1)
  {    
    if(gsm_rx_byte_count > (GSM_RX_BUFFER_SIZE -1))
    {
      gsm_rx_byte_count = 0;
      clear_buffer(gsm_rx_buf, GSM_RX_BUFFER_SIZE);
    }
    gsm_rx_buf[gsm_rx_byte_count++] = gsm_rx_byte;
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&gsm_rx_byte ,1);
  }
}


void copy_buffer(uint8_t *dst, const uint8_t *src, uint16_t size)
{
  while(size)
  {
    size--;
    *dst = *src;
    dst++;
    src++;
  }
}

static void prepare_payload(void)
{
  clear_buffer((uint8_t *)payload, 255);
#if TEST_URL
  payload_len = sprintf(payload,"data=lat:%.06f,lng:%.06f,from_input:%s,location:jaipur\r\n",latitude,longitude,imei);
#else
   latitude = latitude + 1.0f;
  payload_len = sprintf(payload,"lat=%.06f&lng=%.06f&from_input=%s&location=jaipur\r\n",latitude,longitude,imei);
#endif  
}

void gsm_power_on(void)
{
  HAL_GPIO_WritePin(GSM_PWR_KEY_GPIO_Port, GSM_PWR_KEY_Pin, GPIO_PIN_RESET);
  HAL_Delay(1200);
  HAL_GPIO_WritePin(GSM_PWR_KEY_GPIO_Port, GSM_PWR_KEY_Pin,GPIO_PIN_SET);
  HAL_Delay(2000);
}

void gsm_power_off(void)
{
  HAL_GPIO_WritePin(GSM_PWR_KEY_GPIO_Port, GSM_PWR_KEY_Pin, GPIO_PIN_RESET);
  HAL_Delay(1200);
  HAL_GPIO_WritePin(GSM_PWR_KEY_GPIO_Port, GSM_PWR_KEY_Pin,GPIO_PIN_SET);
  HAL_Delay(2000);
}

void check_sim800_modem_ready(void)
{
  uint8_t power_on = 1;
  uint32_t start_time = HAL_GetTick();
  sim800_send_data = 1;
  do{    
      if(sim800_send_data)
      {  
        at_clear_rx_buffer();
        at_receive();
        send_at_cmd("AT\r\n");
        sim800_send_data = 0;
        tim_flag.sim800_timeout_flag = 0;
        sim800_timeout += THREE_SEC_DELAY;
      }   
      
      if(sim800_timer_overflow())
      {
        DBG_MSG("%s\r\n", (char *)gsm_rx_buf);
        sim800_send_data = 1;
        if(strstr((char *)gsm_rx_buf,"AT\r\r\nOK\r\n")>0)
        {
          power_on = 0;
        }       
      }
    
      if((HAL_GetTick()- start_time)> 45*1000)
      {
        gsm_power_off();
        HAL_NVIC_SystemReset();
      }    
   }while(power_on);
}

void send_at_cmd(char *pdata)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)pdata, strlen(pdata), HAL_MAX_DELAY);
}

void clear_buffer(uint8_t *buff, uint16_t size)
{
  while(size)
  {
    size--;
    buff[size] = '\0';
  }
}

struct __FILE { 
int handle; 
};

FILE __stdout;

int fputc(int ch, FILE *f) 
{
  (void) (f);
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 100);
  return ch;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
