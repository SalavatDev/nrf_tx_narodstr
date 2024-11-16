#include "NRF24.h"
//------------------------------------------------
 
//------------------------------------------------
#define TX_ADR_WIDTH 3
#define TX_PLOAD_WIDTH 2
 

#define SPI_DATA_TYPE uint8_t

#define	SPI_SOFT_SCK_Pin	GPIO_PIN_5	
#define	SPI_SOFT_SDI_Pin	GPIO_PIN_6
#define	SPI_SOFT_SDO_Pin	GPIO_PIN_7

#define	SPI_SOFT_SDI_GPIO_Port	GPIOA
#define	SPI_SOFT_SCK_GPIO_Port	GPIOA
#define	SPI_SOFT_SDO_GPIO_Port 	GPIOA


uint8_t TX_ADDRESS[TX_ADR_WIDTH] = {0xb3,0xb4,0x01};
uint8_t RX_BUF[TX_PLOAD_WIDTH] = {0};
 
uint8_t ErrCnt_Fl = 0;

/****************************************************/

void delay_us(uint32_t us) // DelayMicro
{
   
    TIM2->CNT = 0;	
    while(TIM2->CNT < us);
	
}

SPI_DATA_TYPE SPI_send_soft(SPI_DATA_TYPE data)
{
  
  SPI_DATA_TYPE result = 0;
  uint8_t i = sizeof(SPI_DATA_TYPE) * 8;                    
   
  while(i--){
          
    result <<= 1;
    if(data & (0x80)) {       
        SPI_SOFT_SDO_GPIO_Port->BSRR = SPI_SOFT_SDO_Pin;
    }else {
        SPI_SOFT_SDO_GPIO_Port->BRR = SPI_SOFT_SDO_Pin;
    }  
    
    delay_us(1);
    SPI_SOFT_SCK_GPIO_Port->BSRR = SPI_SOFT_SCK_Pin;
		if(SPI_SOFT_SDI_GPIO_Port->IDR & SPI_SOFT_SDI_Pin)
			result |= 1;
    delay_us(1);   
    SPI_SOFT_SCK_GPIO_Port->BRR = SPI_SOFT_SCK_Pin; 
		delay_us(1);
 
 
				
         
    data <<= 1;
		
  }  
  
   
  return result;
  
}

//------------------------------------------------
void DelayMicro(__IO uint32_t us)
{
		TIM2->CNT = 0;	
    while(TIM2->CNT < us);
}
//--------------------------------------------------
uint8_t NRF24_ReadReg(uint8_t addr)
{
  uint8_t dt=0, cmd;
  CS_ON;
  dt = SPI_send_soft(addr);
  if (addr!=STATUS)//если адрес равен адрес регистра статус то и возварщаем его состояние
  {
    cmd = 0xFF;
    dt = SPI_send_soft(cmd);
  }
  CS_OFF;
  return dt;
}
//------------------------------------------------
void NRF24_WriteReg(uint8_t addr, uint8_t dt)
{
  addr |= W_REGISTER;//включим бит записи в адрес
  CS_ON;
  SPI_send_soft(addr);//отправим адрес в шину
  SPI_send_soft(dt);//отправим данные в шину
  CS_OFF;
}
//------------------------------------------------
void NRF24_ToggleFeatures(void)
{
  uint8_t dt[1] = {ACTIVATE};
  CS_ON;
  SPI_send_soft(dt[0]);
  DelayMicro(1);
  dt[0] = 0x73;
  SPI_send_soft(dt[0]);
  CS_OFF;
}
//-----------------------------------------------
void NRF24_Read_Buf(uint8_t addr,uint8_t *pBuf,uint8_t bytes)
{
  CS_ON;
  SPI_send_soft( addr );//отправим адрес в шину
	for(uint8_t i=0; i<bytes; ++i){
		SPI_send_soft(pBuf[i]);//отправим данные в буфер
	}
  
  CS_OFF;
}
//------------------------------------------------
void NRF24_Write_Buf(uint8_t addr,uint8_t *pBuf,uint8_t bytes)
{
  addr |= W_REGISTER;//включим бит записи в адрес
  CS_ON;
  SPI_send_soft(addr);//отправим адрес в шину
  DelayMicro(1);	
	for(uint8_t i=0; i<bytes; ++i){
		SPI_send_soft(pBuf[i]);//отправим данные в буфер
	}
	
  CS_OFF;
}
//------------------------------------------------
void NRF24_FlushRX(void)
{
  uint8_t dt[1] = {FLUSH_RX};
  CS_ON;
  SPI_send_soft(dt[0]);
  DelayMicro(1);
  CS_OFF;
}
//------------------------------------------------
void NRF24_FlushTX(void)
{
  uint8_t dt[1] = {FLUSH_TX};
  CS_ON;
  SPI_send_soft(dt[0]);
  DelayMicro(1);
  CS_OFF;
}
//------------------------------------------------
void NRF24L01_RX_Mode(void)
{
  uint8_t regval=0x00;
  regval = NRF24_ReadReg(CONFIG);
  //разбудим модуль и переведём его в режим приёмника, включив биты PWR_UP и PRIM_RX
  regval |= (1<<PWR_UP)|(1<<PRIM_RX);
  NRF24_WriteReg(CONFIG,regval);
  CE_SET;
  DelayMicro(150); //Задержка минимум 130 мкс
  // Flush buffers
  NRF24_FlushRX();
  NRF24_FlushTX();
}
//------------------------------------------------
void NRF24L01_TX_Mode(uint8_t *pBuf)
{
  NRF24_Write_Buf(TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);
  CE_RESET;
  // Flush buffers
  NRF24_FlushRX();
  NRF24_FlushTX();
}
//------------------------------------------------
void NRF24_Transmit(uint8_t addr,uint8_t *pBuf,uint8_t bytes)
{
  CE_RESET;
  CS_ON;
  SPI_send_soft(addr);//отправим адрес в шину
  DelayMicro(1);
 
	for(uint8_t i=0; i<bytes; ++i){
		SPI_send_soft(pBuf[i]);//отправим данные в буфер
	}
  CS_OFF;
  CE_SET;
}
//------------------------------------------------
uint8_t NRF24L01_Send(uint8_t *pBuf)
{
  uint8_t status=0x00, regval=0x00;
	NRF24L01_TX_Mode(pBuf);
	regval = NRF24_ReadReg(CONFIG);
	//если модуль ушел в спящий режим, то разбудим его, включив бит PWR_UP и выключив PRIM_RX
	regval |= (1<<PWR_UP);
	regval &= ~(1<<PRIM_RX);
	NRF24_WriteReg(CONFIG,regval);
	DelayMicro(150); //Задержка минимум 130 мкс
	//Отправим данные в воздух
	NRF24_Transmit(WR_TX_PLOAD, pBuf, TX_PLOAD_WIDTH);
	CE_SET;
	DelayMicro(15); //minimum 10us high pulse (Page 21)
	CE_RESET;
	while((GPIO_PinState)IRQ == GPIO_PIN_SET) {}
	status = NRF24_ReadReg(STATUS);
	if(status&TX_DS) //tx_ds == 0x20
	{
		LED_TGL;
		NRF24_WriteReg(STATUS, 0x20);
	}
	else if(status&MAX_RT)
	{
		NRF24_WriteReg(STATUS, 0x10);
		NRF24_FlushTX();
	}
	regval = NRF24_ReadReg(OBSERVE_TX);
	//Уходим в режим приёмника
  NRF24L01_RX_Mode();
	return regval;
}
//------------------------------------------------
/*
char str_rx[64];
void NRF24L01_Receive(void)
{
  uint8_t status=0x01;
  uint16_t dt=0;
	while((GPIO_PinState)IRQ == GPIO_PIN_SET) {}
	status = NRF24_ReadReg(STATUS);
	 sprintf(str_rx,"STATUS: 0x%02X\r\n",status);
	//HAL_UART_Transmit(&huart2,(uint8_t*)str1,strlen(str1),0x1000);
	LED_TGL;
  DelayMicro(10);
  status = NRF24_ReadReg(STATUS);
  if(status & 0x40)
  {
    NRF24_Read_Buf(RD_RX_PLOAD,RX_BUF,TX_PLOAD_WIDTH);
    dt = *(int16_t*)RX_BUF;
    //Clear_7219();
   // Number_7219(dt);
    dt = *(int16_t*)(RX_BUF+2);
    //NumberL_7219(dt);
    NRF24_WriteReg(STATUS, 0x40);
  }	
}
*/

uint8_t isChipConnected(void)
{
	uint8_t setup = NRF24_ReadReg(SETUP_AW);

	if(setup >= 1 && setup <= 3)
	{
		return 1;
	}

	return 0;
}
//------------------------------------------------
void NRF24_ini(void)
{
	CE_RESET;
  DelayMicro(5000);
	NRF24_WriteReg(CONFIG, 0x0a); // Set PWR_UP bit, enable CRC(1 byte) &Prim_RX:0 (Transmitter)
  DelayMicro(5000);
	NRF24_WriteReg(EN_AA, 0x03); // Enable Pipe0
	NRF24_WriteReg(EN_RXADDR, 0x03); // Enable Pipe0
	NRF24_WriteReg(SETUP_AW, 0x01); // Setup address width=3 bytes
	NRF24_WriteReg(SETUP_RETR, 0x5F); // // 1500us, 15 retrans
	NRF24_ToggleFeatures();
	NRF24_WriteReg(FEATURE, 0);
	NRF24_WriteReg(DYNPD, 0);
	NRF24_WriteReg(STATUS, 0x70); //Reset flags for IRQ
	NRF24_WriteReg(RF_CH, 76); // частота 2476 MHz
	NRF24_WriteReg(RF_SETUP, 0x06); //TX_PWR:0dBm, Datarate:1Mbps
	NRF24_Write_Buf(TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);
	NRF24_Write_Buf(RX_ADDR_P1, TX_ADDRESS, TX_ADR_WIDTH);
	NRF24_WriteReg(RX_PW_P0, TX_PLOAD_WIDTH); //Number of bytes in RX payload in data pipe 0
 //пока уходим в режим приёмника
  NRF24L01_RX_Mode();
  LED_OFF;
}
//--------------------------------------------------
