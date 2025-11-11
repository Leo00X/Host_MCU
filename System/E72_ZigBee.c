#include "E72_ZigBee.h"
#include <string.h> // 用于 memcpy

/*
 ********************************************************************************************************
 * 私有类型定义
 ********************************************************************************************************
 */

/**
 * @brief 串口接收状态机状态
 */
typedef enum
{
  E72_RX_STATE_WAIT_HEADER = 0, // 等待 0x55 帧头
  E72_RX_STATE_WAIT_LEN,        // 等待 长度字节
  E72_RX_STATE_WAIT_DATA        // 接收数据 (Type + Cmd + Payload + FCS)
} E72_RxState_Enum;


/*
 ********************************************************************************************************
 * 私有静态变量
 ********************************************************************************************************
 */

// 上层应用注册的帧接收回调函数
static e72_frame_rx_callback_t s_e72_frame_rx_callback = NULL;

// 串口接收状态机
static E72_RxState_Enum s_e72_rx_state = E72_RX_STATE_WAIT_HEADER;
static u8  s_u8RxBuffer[E72_MAX_FRAME_LEN]; // 接收缓冲区
static u16 s_u16RxCount = 0;                // 当前已接收字节数
static u8  s_u8FrameDataLen = 0;            // 当前帧的数据长度 (即Len字段的值)


/*
 ********************************************************************************************************
 * 私有函数原型
 ********************************************************************************************************
 */

/**
 * @brief 计算E72 HEX帧的FCS校验和。
 * @note  根据规范P6: FCS为 "Len 到 Payload 的异或和"。
 * @param pData: 指向"Len"字段的指针。
 * @param u8Len: "Len"字段到Payload末尾的总长度 (即 Len字段的值 + 1)。
 * @retval 8位的FCS校验和
 */
static u8 _E72_CalcFCS(const u8* pData, u8 u8Len);

/**
 * @brief 内部串口接收回调函数 (将被注册到 bsp_uart)。
 * @note  这是接收状态机的核心。
 * @param u8Data: 串口接收到的单个字节。
 * @retval 无
 */
static void _E72_RxCallback(u8 u8Data);

/**
 * @brief 内部帧解析函数。
 * @note  当接收状态机确认收到完整一帧后，调用此函数进行FCS校验和解析。
 * @retval 无
 */
static void _E72_ParseFrame(void);


/*
 ********************************************************************************************************
 * 公共函数定义
 ********************************************************************************************************
 */

/**
 * @brief (新) 向Zigbee网络中的指定节点发送“透传”数据。
 */
void E72_SendTransparentData(u16 u16DestAddr, u8 u8Endpoint, const u8* pData, u8 u8DataLen)
{
    // E72的Payload缓冲区
    // 格式: [短地址L][短地址H][端口][你的应用数据...]
    u8 e72_payload_buf[E72_MAX_PAYLOAD_LEN];
    u8 e72_payload_len = 0;

    // 1. 打包 E72 的“透传Payload”
    //    根据规范P18 (HEX规范 V1.7):
    //    透传Payload = 目标短地址(2B) + 目标端口(1B) + 数据(...)
    
    // 目标短地址 (小端)
    e72_payload_buf[0] = (u8)(u16DestAddr);      // Addr Low
    e72_payload_buf[1] = (u8)(u16DestAddr >> 8); // Addr High
    
    // 目标端口
    e72_payload_buf[2] = u8Endpoint;
    
    // 2. 拷贝你的应用数据 (0xAA...0x55 帧)
    if (pData != NULL && u8DataLen > 0)
    {
        // 确保不会溢出E72的Payload缓冲区 (E72_MAX_PAYLOAD_LEN - 3个字节的头部)
        if (u8DataLen > (E72_MAX_PAYLOAD_LEN - 3))
        {
            u8DataLen = E72_MAX_PAYLOAD_LEN - 3; // 截断保护
        }
        memcpy(&e72_payload_buf[3], pData, u8DataLen);
    }
    
    // 3. 计算 E72 的 Payload 总长度
    e72_payload_len = 3 + u8DataLen;

    // 4. 调用底层 HEX 命令发送
    E72_SendHexCommand(
        E72_CMD_TYPE_PASSTHROUGH,       // 0x0A
        E72_CMD_ID_PASSTHROUGH_DATA,    // 0x0001
        e72_payload_buf,                // E72的Payload
        e72_payload_len                 // E72的Payload长度
    );
}

/**
 * @brief 初始化E72 Zigbee驱动。
 */
void E72_Init(void)
{
  // 1. 初始化串口 (通过宏调用 bsp_uart.c 中的函数)
  E72_UART_INIT();
  
  // 2. 注册内部的串口接收回调函数
  E72_UART_REGISTER_RX_CALLBACK(_E72_RxCallback);
  
  // 3. 复位接收状态机
  s_e72_rx_state = E72_RX_STATE_WAIT_HEADER;
  s_u16RxCount = 0;
  s_e72_frame_rx_callback = NULL;
}

/**
 * @brief 注册一个上层回调函数，当收到E72模组的完整数据帧时将调用该函数。
 */
void E72_Register_Frame_Callback(e72_frame_rx_callback_t callback)
{
  s_e72_frame_rx_callback = callback;
}

/**
 * @brief (底层) 向E72模组发送一帧HEX指令。
 */
void E72_SendHexCommand(u8 u8Type, u16 u16Cmd, const u8* pPayload, u8 u8PayloadLen)
{
  u8 txBuffer[E72_MAX_FRAME_LEN];
  u8 u8FrameLen; // Len 字段的值
  u8 u8TotalLen; // 整帧的字节数
  u8 i;
  
  // 规范P6: Len = Type(1) + Cmd(2) + PayloadLen
  u8FrameLen = 1 + 2 + u8PayloadLen;
  
  // 1. 组包
  txBuffer[0] = E72_FRAME_HEADER;       // Header
  txBuffer[1] = u8FrameLen;             // Len
  txBuffer[2] = u8Type;                 // Type
  txBuffer[3] = (u8)(u16Cmd >> 8);      // Cmd (High Byte)
  txBuffer[4] = (u8)(u16Cmd);           // Cmd (Low Byte)
  
  // 2. 拷贝 Payload
  if (pPayload != NULL && u8PayloadLen > 0)
  {
    memcpy(&txBuffer[5], pPayload, u8PayloadLen);
  }
  
  // 3. 计算 FCS (从 Len 字段开始计算)
  //    FCS校验的数据长度 = Len(1) + Type(1) + Cmd(2) + PayloadLen = 1 + u8FrameLen
  u8 u8Fcs = _E72_CalcFCS(&txBuffer[1], (u8FrameLen + 1));
  
  // 4. 填充 FCS (位于 Payload 之后)
  txBuffer[5 + u8PayloadLen] = u8Fcs;
  
  // 5. 计算整帧总长度 (Header + Len + Data(u8FrameLen) + FCS)
  u8TotalLen = 1 + 1 + u8FrameLen + 1;
  
  // 6. 发送数据
  for (i = 0; i < u8TotalLen; i++)
  {
    E72_UART_SEND_BYTE(txBuffer[i]);
  }
}


/*
 ********************************************************************************************************
 * 私有函数定义
 ********************************************************************************************************
 */

/**
 * @brief 计算E72 HEX帧的FCS校验和。
 */
static u8 _E72_CalcFCS(const u8* pData, u8 u8Len)
{
  u8 u8Fcs = 0;
  u8 i;
  
  for (i = 0; i < u8Len; i++)
  {
    u8Fcs ^= pData[i];
  }
  return u8Fcs;
}

/**
 * @brief 内部串口接收回调函数 (将被注册到 bsp_uart)。
 */
static void _E72_RxCallback(u8 u8Data)
{
  switch (s_e72_rx_state)
  {
    /* 1. 等待帧头 */
    case E72_RX_STATE_WAIT_HEADER:
    {
      if (u8Data == E72_FRAME_HEADER)
      {
        s_u16RxCount = 0;
        s_u8RxBuffer[s_u16RxCount++] = u8Data; // s_u8RxBuffer[0] = 0x55
        s_e72_rx_state = E72_RX_STATE_WAIT_LEN;
      }
      break;
    }
    
    /* 2. 等待长度 */
    case E72_RX_STATE_WAIT_LEN:
    {
      s_u8FrameDataLen = u8Data; // Len 字段的值
      s_u8RxBuffer[s_u16RxCount++] = u8Data; // s_u8RxBuffer[1] = Len
      
      // 检查长度是否合法 (最短帧Len=3, 即Type+CmdH+CmdL)
      if (s_u8FrameDataLen < 3 || s_u8FrameDataLen > (E72_MAX_PAYLOAD_LEN + 3))
      {
        // 长度异常，复位状态机
        s_e72_rx_state = E72_RX_STATE_WAIT_HEADER;
      }
      else
      {
        s_e72_rx_state = E72_RX_STATE_WAIT_DATA;
      }
      break;
    }
    
    /* 3. 接收数据 (Type + Cmd + Payload + FCS) */
    case E72_RX_STATE_WAIT_DATA:
    {
      s_u8RxBuffer[s_u16RxCount++] = u8Data;
      
      // 检查是否已收满一帧
      // 我们需要接收 Len + FCS 总共 (s_u8FrameDataLen + 1) 个字节
      // s_u16RxCount 从 2 开始计数 (Type是第2个)
      // 当 s_u16RxCount == (s_u8FrameDataLen + 1 + 2) 时收满
      // 即 s_u16RxCount == s_u8FrameDataLen + 3
      if (s_u16RxCount >= (s_u8FrameDataLen + 3))
      {
        _E72_ParseFrame(); // 收到完整一帧，进行解析
        s_e72_rx_state = E72_RX_STATE_WAIT_HEADER; // 复位状态机
      }
      
      // 额外保护，防止缓冲区溢出
      if (s_u16RxCount >= E72_MAX_FRAME_LEN)
      {
          s_e72_rx_state = E72_RX_STATE_WAIT_HEADER;
      }
      break;
    }
    
    default:
    {
      s_e72_rx_state = E72_RX_STATE_WAIT_HEADER;
      break;
    }
  }
}

/**
 * @brief 内部帧解析函数。
 */
static void _E72_ParseFrame(void)
{
  u8 u8CalcFcs;
  u8 u8RecvFcs;
  
  // 1. 计算FCS (从 s_u8RxBuffer[1] (即Len字段) 开始)
  //    校验长度 = Len(1) + Data(s_u8FrameDataLen) = s_u8FrameDataLen + 1
  u8CalcFcs = _E72_CalcFCS(&s_u8RxBuffer[1], s_u8FrameDataLen + 1);
  
  // 2. 获取接收到的FCS (位于整帧的最后一个字节)
  //    总长度 = Header(1) + Len(1) + Data(s_u8FrameDataLen) + FCS(1) = s_u8FrameDataLen + 3
  u8RecvFcs = s_u8RxBuffer[s_u8FrameDataLen + 2];
  
  // 3. 校验FCS
  if (u8CalcFcs == u8RecvFcs)
  {
    // FCS 校验成功
    if (s_e72_frame_rx_callback != NULL)
    {
      u8  u8Type;
      u16 u16Cmd;
      u8* pPayload;
      u8  u8PayloadLen;
      
      // 提取 Type (s_u8RxBuffer[2])
      u8Type = s_u8RxBuffer[2];
      
      // 提取 Cmd (s_u8RxBuffer[3] H, s_u8RxBuffer[4] L)
      u16Cmd = ((u16)s_u8RxBuffer[3] << 8) | s_u8RxBuffer[4];
      
      // 提取 Payload (s_u8RxBuffer[5] 开始)
      pPayload = &s_u8RxBuffer[5];
      
      // 提取 PayloadLen (Len - Type(1) - Cmd(2))
      u8PayloadLen = s_u8FrameDataLen - 3;
      
      // 4. 调用上层回调
      s_e72_frame_rx_callback(u8Type, u16Cmd, pPayload, u8PayloadLen);
    }
  }
  else
  {
    // FCS 校验失败 (可以在这里添加一个错误计数器或Log)
    // uprintf("E72 FCS Error!\r\n");
  }
}
