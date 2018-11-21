/**********************************************************************************************************************
File: user_app.c                                                                

----------------------------------------------------------------------------------------------------------------------
To start a new task using this user_app as a template:
 1. Copy both user_app.c and user_app.h to the Application directory
 2. Rename the files yournewtaskname.c and yournewtaskname.h
 3. Add yournewtaskname.c and yournewtaskname.h to the Application Include and Source groups in the IAR project
 4. Use ctrl-h (make sure "Match Case" is checked) to find and replace all instances of "user_app" with "yournewtaskname"
 5. Use ctrl-h to find and replace all instances of "UserApp1" with "YourNewTaskName"
 6. Use ctrl-h to find and replace all instances of "USER_APP" with "YOUR_NEW_TASK_NAME"
 7. Add a call to YourNewTaskNameInitialize() in the init section of main
 8. Add a call to YourNewTaskNameRunActiveState() in the Super Loop section of main
 9. Update yournewtaskname.h per the instructions at the top of yournewtaskname.h
10. Delete this text (between the dashed lines) and update the Description below to describe your task
----------------------------------------------------------------------------------------------------------------------

Description:
This is a user_app.c file template 

------------------------------------------------------------------------------------------------------------------------
API:

Public functions:


Protected System functions:
void UserApp1Initialize(void)
Runs required initialzation for the task.  Should only be called once in main init section.

void UserApp1RunActiveState(void)
Runs current task state.  Should only be called once in main loop.


**********************************************************************************************************************/

#include "configuration.h"
u8 au8Button1=0;
/***********************************************************************************************************************
Global variable definitions with scope across entire project.
All Global variable names shall start with "G_"
***********************************************************************************************************************/
/* New variables */
volatile u32 G_u32UserApp1Flags;                       /* Global state flags */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern u32 G_u32AntApiCurrentDataTimeStamp;                       /* From ant_api.c */
extern AntApplicationMessageType G_eAntApiCurrentMessageClass;    /* From ant_api.c */
extern u8 G_au8AntApiCurrentMessageBytes[ANT_APPLICATION_MESSAGE_BYTES];  /* From ant_api.c */
extern AntExtendedDataType G_sAntApiCurrentMessageExtData;        /* From ant_api.c */

extern volatile u32 G_u32SystemFlags;                  /* From main.c */
extern volatile u32 G_u32ApplicationFlags;             /* From main.c */

extern volatile u32 G_u32SystemTime1ms;                /* From board-specific source file */
extern volatile u32 G_u32SystemTime1s;                 /* From board-specific source file */



/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "UserApp1_" and be declared as static.
***********************************************************************************************************************/
static u32 UserApp1_u32DataMsgCount = 0;             /* Counts the number of ANT_DATA packets received */
static u32 UserApp1_u32TickMsgCount = 0;             /* Counts the number of ANT_TICK packets received */

static fnCode_type UserApp1_StateMachine;            /* The state machine function pointer */
static u32 UserApp1_u32Timeout;                      /* Timeout counter used across states */


/**********************************************************************************************************************
Function Definitions
**********************************************************************************************************************/

/*--------------------------------------------------------------------------------------------------------------------*/
/* Public functions                                                                                                   */
/*--------------------------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------------------------------------*/
/* Protected functions                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------------------
Function: UserApp1Initialize

Description:
Initializes the State Machine and its variables.

Requires:
  -

Promises:
  - 
*/
void UserApp1Initialize(void)
{
  u8 au8WelcomeMessage[] = "ARE YOU OK?";
  u8 au8Instructions[] = "Press the damm B0";
  LedPWM(LCD_RED, LED_PWM_0);
  
  AntAssignChannelInfoType sAntSetupData;
  
  /* Clear screen and place start messages */

  LCDCommand(LCD_CLEAR_CMD);
  LCDMessage(LINE1_START_ADDR, au8WelcomeMessage); 
  LCDMessage(LINE2_START_ADDR, au8Instructions); 

  /* Start with LED0 in RED state = channel is not configured */
  LedOn(RED);

  
 /* Configure ANT for this application */
  sAntSetupData.AntChannel          = ANT_CHANNEL_USERAPP;
  sAntSetupData.AntChannelType      = ANT_CHANNEL_TYPE_USERAPP;
  sAntSetupData.AntChannelPeriodLo  = ANT_CHANNEL_PERIOD_LO_USERAPP;
  sAntSetupData.AntChannelPeriodHi  = ANT_CHANNEL_PERIOD_HI_USERAPP;
  
  sAntSetupData.AntDeviceIdLo       = ANT_DEVICEID_LO_USERAPP;
  sAntSetupData.AntDeviceIdHi       = ANT_DEVICEID_HI_USERAPP;
  sAntSetupData.AntDeviceType       = ANT_DEVICE_TYPE_USERAPP;
  sAntSetupData.AntTransmissionType = ANT_TRANSMISSION_TYPE_USERAPP;
  sAntSetupData.AntFrequency        = ANT_FREQUENCY_USERAPP;
  sAntSetupData.AntTxPower          = ANT_TX_POWER_USERAPP;

  sAntSetupData.AntNetwork = ANT_NETWORK_DEFAULT;

    sAntSetupData.AntNetworkKey[0] = 0xB9;
    sAntSetupData.AntNetworkKey[1] = 0xA5;
    sAntSetupData.AntNetworkKey[2] = 0x21;
    sAntSetupData.AntNetworkKey[3] = 0xFB;
    sAntSetupData.AntNetworkKey[4] = 0xBD;
    sAntSetupData.AntNetworkKey[5] = 0x72;
    sAntSetupData.AntNetworkKey[6] = 0xC3;
    sAntSetupData.AntNetworkKey[7] = 0x45;



    
  /* If good initialization, set state to Idle */
  if( AntAssignChannel(&sAntSetupData) )
  {
    /* Channel assignment is queued so start timer */
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    LedOn(RED);
    UserApp1_StateMachine = UserApp1SM_WaitChannelAssign;
  }
  else
  {
    /* The task isn't properly initialized, so shut it down and don't run */

    LedBlink(RED, LED_4HZ);
    UserApp1_StateMachine = UserApp1SM_Error;
  }

} /* end UserApp1Initialize() */


/*----------------------------------------------------------------------------------------------------------------------
Function UserApp1RunActiveState()

Description:
Selects and runs one iteration of the current state in the state machine.
All state machines have a TOTAL of 1ms to execute, so on average n state machines
may take 1ms / n to execute.

Requires:
  - State machine function pointer points at current state

Promises:
  - Calls the function to pointed by the state machine function pointer
*/
void UserApp1RunActiveState(void)
{
  UserApp1_StateMachine();

} /* end UserApp1RunActiveState */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions                                                                                                  */
/*--------------------------------------------------------------------------------------------------------------------*/


/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for the ANT channel assignment to finish */
static void UserApp1SM_WaitChannelAssign(void)
{
  /* Check if the channel assignment is complete */
  if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP) == ANT_CONFIGURED)
  {
    LedOff(RED);
    LedOn(YELLOW);
    UserApp1_StateMachine = UserApp1SM_Idle;
  }
  
  /* Monitor for timeout */
  if( IsTimeUp(&UserApp1_u32Timeout, 5000) )
  {
    DebugPrintf("\n\r***Channel FUCKUP!***\n\n\r");
    UserApp1_StateMachine = UserApp1SM_Error;
  }
      
} /* end UserApp1SM_WaitChannelAssign() */


  /*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for a message to be queued */
static void UserApp1SM_Idle(void)
{
  /* Look for BUTTON 0 to open channel */
  if(WasButtonPressed(BUTTON0))
  {
    /* Got the button, so complete one-time actions before next state */
    ButtonAcknowledge(BUTTON0);
    
    /* Queue open channel and change LED0 from yellow to blinking green to indicate channel is opening */
    AntOpenChannelNumber(ANT_CHANNEL_USERAPP);


    LedOff(YELLOW);
    LedBlink(GREEN, LED_2HZ);
    LedOff(RED);
    //LedBlink(GREEN);

    
    /* Set timer and advance states */
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_WaitChannelOpen;
  }
    
} /* end UserApp1SM_Idle() */
     

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for channel to open */
static void UserApp1SM_WaitChannelOpen(void)
{
  /* Monitor the channel status to check if channel is opened */
  if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP) == ANT_OPEN)
  {

    LedOn(GREEN);
    LedOn(BLUE);
    
    UserApp1_StateMachine = UserApp1SM_ChannelOpen;
  }
  
  /* Check for timeout */
  if( IsTimeUp(&UserApp1_u32Timeout, TIMEOUT_VALUE) )
  {
    AntCloseChannelNumber(ANT_CHANNEL_USERAPP);

    LedOn(ORANGE);
    LedOn(RED);
    
    UserApp1_StateMachine = UserApp1SM_Idle;
  }
    
} /* end UserApp1SM_WaitChannelOpen() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* Channel is open, so monitor data */
static void UserApp1SM_ChannelOpen(void)
{
  static u8 u8LastState = 0xff;
  static u8 au8TickMessage[] = "EVENT x\n\r";  /* "x" at index [6] will be replaced by the current code */
  static u8 au8DataContent[] = "FFFFFFFFFFFFFFFF";
  static u8 au8LastAntData[ANT_APPLICATION_MESSAGE_BYTES] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  static u8 au8TestMessage[] = {0, 0, 0, 0, 0xA5, 0, 0, 0};
  static u8 au8HeartRate[]="HeartRate:   bpm";
  static u8 au8HeartRateHex[]="   ";
  u8 au8Dec;
  
  bool bGotNewData;

  /* Check for BUTTON0 to close channel */
  if(WasButtonPressed(BUTTON0))
  {
    /* Got the button, so complete one-time actions before next state */
    ButtonAcknowledge(BUTTON0);
    
    /* Queue close channel and change LED to blinking green to indicate channel is closing */
    AntCloseChannelNumber(ANT_CHANNEL_USERAPP);
    u8LastState = 0xff;

    LedOff(YELLOW);
    LedOff(BLUE);
    LedBlink(GREEN, LED_2HZ);
    
    /* Set timer and advance states */
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_WaitChannelClose;
  } /* end if(WasButtonPressed(BUTTON0)) */
  
  /* Always check for ANT messages */
  if( AntReadAppMessageBuffer() )
  {
     /* New data message: check what it is */
    if(G_eAntApiCurrentMessageClass == ANT_DATA)
    {
      UserApp1_u32DataMsgCount++;
      
      /* We are synced with a device, so blue is solid */
      LedOff(GREEN);
      LedOn(BLUE);

      /* Check if the new data is the same as the old data and update as we go */
      bGotNewData = FALSE;
      for(u8 i = 0; i < ANT_APPLICATION_MESSAGE_BYTES; i++)
      {
        if(G_au8AntApiCurrentMessageBytes[i] != au8LastAntData[i])
        {
          bGotNewData = TRUE;
          au8LastAntData[i] = G_au8AntApiCurrentMessageBytes[i];

          au8DataContent[2 * i]     = HexToASCIICharUpper(G_au8AntApiCurrentMessageBytes[i] / 16);
          au8DataContent[2 * i + 1] = HexToASCIICharUpper(G_au8AntApiCurrentMessageBytes[i] % 16); 
        }
      }
      
      if(bGotNewData)
      {
        /* We got new data: show on LCD */
        LCDClearChars(LINE2_START_ADDR, 20); 
        au8HeartRateHex[1]=au8DataContent[14];
        au8HeartRateHex[2]=au8DataContent[15];
        
        /*十六进制到十进制*/
        if(au8HeartRateHex[1]>='A' && au8HeartRateHex[1]<='F')
        {
           au8Dec=(au8HeartRateHex[1]-'A')*16+144;
        }
        else
        {
          au8Dec=(au8HeartRateHex[1]-'0')*16;
        }       
        if(au8HeartRateHex[2]>='A' && au8HeartRateHex[2]<='F')
        {
           au8Dec=au8Dec+(au8HeartRateHex[2]-'A')+9;
        }
        else
        {
          au8Dec=au8Dec+(au8HeartRateHex[2]-'0');
        }
        
        //LCDMessage(LINE2_START_ADDR, u8Dec);
        if(au8Dec>=150||au8Dec<40)
        {
          u8 au8Burst[] = "YOU WILL BURST ";
          LCDMessage(LINE1_START_ADDR, au8Burst);
          LedBlink(RED, LED_8HZ);
          
          
        }
        else if(au8Dec>=120)
        {
          u8 au8Fast[] = "ARE YOU RUNNING?";
          LCDMessage(LINE1_START_ADDR,au8Fast);
          LedBlink(RED, LED_4HZ);
          
        }
        else if(au8Dec>=100)
        {
          u8 au8BitFast[] = "ARE YOU NERVOUS?";
          LCDMessage(LINE1_START_ADDR,au8BitFast);
          LedBlink(RED, LED_2HZ);
          
        }
        else
        {
          u8 au8Easy[] = "Everything is ok";
          LCDMessage(LINE1_START_ADDR,au8Easy);
          LedOff(RED);
          
        }
        
        
        au8HeartRate[12]=au8Dec%10+'0';
        au8Dec/=10;
        au8HeartRate[11]=au8Dec%10+'0';
        au8Dec/=10;
        au8HeartRate[10]=au8Dec+'0';
        
        
        
        LCDMessage(LINE2_START_ADDR,au8HeartRate ); 

        /* Update our local message counter and send the message back */
        au8TestMessage[7]++;
        if(au8TestMessage[7] == 0)
        {
          au8TestMessage[6]++;
          if(au8TestMessage[6] == 0)
          {
            au8TestMessage[5]++;
          }
        }
        AntQueueBroadcastMessage(ANT_CHANNEL_USERAPP, au8TestMessage);

        /* Check for a special packet and respond */
        if(G_au8AntApiCurrentMessageBytes[0] == 0xA5)
        {
          LedOff(LCD_RED);
          LedOff(LCD_GREEN);
          LedOff(LCD_BLUE);
          
          if(G_au8AntApiCurrentMessageBytes[1] == 1)
          {
            LedOn(LCD_RED);
          }
          
          if(G_au8AntApiCurrentMessageBytes[2] == 1)
          {
            LedOn(LCD_GREEN);
          }

          if(G_au8AntApiCurrentMessageBytes[3] == 1)
          {
            LedOn(LCD_BLUE);
          }
        }

    

      }
    }
    else if(G_eAntApiCurrentMessageClass == ANT_TICK)
    {
      UserApp1_u32TickMsgCount++;

      /* Look at the TICK contents to check the event code and respond only if it's different */
      if(u8LastState != G_au8AntApiCurrentMessageBytes[ANT_TICK_MSG_EVENT_CODE_INDEX])
      {
        /* The state changed so update u8LastState and queue a debug message */
        u8LastState = G_au8AntApiCurrentMessageBytes[ANT_TICK_MSG_EVENT_CODE_INDEX];
        au8TickMessage[6] = HexToASCIICharUpper(u8LastState);
        DebugPrintf(au8TickMessage);

        /* Parse u8LastState to update LED status */
        switch (u8LastState)
        {

          /* If we are paired but missing messages, blue blinks */
          case EVENT_RX_FAIL:
          {
            LedOff(GREEN);
            LedBlink(BLUE, LED_2HZ);
            break;
          }

          /* If we drop to search, LED is green */
          case EVENT_RX_FAIL_GO_TO_SEARCH:
          {
            LedOff(BLUE);
            LedOn(GREEN);
            break;
          }

          /* If the search times out, the channel should automatically close */
          case EVENT_RX_SEARCH_TIMEOUT:
          {
            DebugPrintf("Search timeout event\r\n");
            break;
          }

          case EVENT_CHANNEL_CLOSED:
          {
            DebugPrintf("Channel closed event\r\n");
            break;
          }

            default:
          {
            DebugPrintf("Unexpected Event\r\n");
            break;
          }
        } /* end switch (G_au8AntApiCurrentMessageBytes) */
      } /* end if (u8LastState != G_au8AntApiCurrentMessageBytes[ANT_TICK_MSG_EVENT_CODE_INDEX]) */
    } /* end else if(G_eAntApiCurrentMessageClass == ANT_TICK) */
    
  } /* end AntReadAppMessageBuffer() */
  
  /* A slave channel can close on its own, so explicitly check channel status */
  if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP) != ANT_OPEN)
  {

    LedBlink(GREEN, LED_2HZ);
    LedOff(BLUE);

    u8LastState = 0xff;
    
    UserApp1_u32Timeout = G_u32SystemTime1ms;
    UserApp1_StateMachine = UserApp1SM_WaitChannelClose;
  } /* if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP) != ANT_OPEN) */
      
} /* end UserApp1SM_ChannelOpen() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for channel to close */
static void UserApp1SM_WaitChannelClose(void)
{
  /* Monitor the channel status to check if channel is closed */
  if(AntRadioStatusChannel(ANT_CHANNEL_USERAPP) == ANT_CLOSED)
  {
    LedOff(GREEN);
    LedOn(YELLOW);

    UserApp1_StateMachine = UserApp1SM_Idle;
  }
  
  /* Check for timeout */
  if( IsTimeUp(&UserApp1_u32Timeout, TIMEOUT_VALUE) )
  {
    LedOff(GREEN);
    LedOff(YELLOW);
    LedBlink(RED, LED_4HZ);

    UserApp1_StateMachine = UserApp1SM_Error;
  }
    
} /* end UserApp1SM_WaitChannelClose() */


/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void UserApp1SM_Error(void)          
{

} /* end UserApp1SM_Error() */



/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
