
#include "Joystick.h"

static uint8_t PrevJoystickHIDReportBuffer[sizeof(USB_JoystickReport_Data_t)];

USB_ClassInfo_HID_Device_t Joystick_HID_Interface = {
  .Config = {
    .InterfaceNumber = INTERFACE_ID_Joystick,
    .ReportINEndpoint = {
      .Address              = JOYSTICK_EPADDR,
      .Size                 = JOYSTICK_EPSIZE,
      .Banks                = 1,
    },
    .PrevReportINBuffer           = PrevJoystickHIDReportBuffer,
    .PrevReportINBufferSize       = sizeof(PrevJoystickHIDReportBuffer),
  },
};

#define OUTPUT_ENABLE_PIN	4
#define SELECT_PIN		5

#define PORTB_INPUTS_MASK	0x0F
#define PORTB_OUTPUTS_MASK	((1<<OUTPUT_ENABLE_PIN)|(1<<SELECT_PIN))

#define enable_controller_chip()	(PORTB &= ~(_BV(OUTPUT_ENABLE_PIN)))
#define disable_controller_chip()	(PORTB |= _BV(OUTPUT_ENABLE_PIN))
#define set_select_to_high()		(PORTB |= _BV(SELECT_PIN))
#define set_select_to_low()		(PORTB &= ~(_BV(SELECT_PIN)))

typedef union {
  uint8_t raw;
  struct {
    unsigned int up:1;
    unsigned int right:1;
    unsigned int down:1;
    unsigned int left:1;
    
    unsigned int pad:4;
  } high;
  struct {
    unsigned int I_btn:1;
    unsigned int II_btn:1;
    unsigned int select:1;
    unsigned int run:1;
    
    unsigned int pad:4;
  } low;
} portb_defs;

int main(void) {
  SetupHardware();
  
  GlobalInterruptEnable();
  
  enable_controller_chip();
  set_select_to_high();
  for (;;) {
    HID_Device_USBTask(&Joystick_HID_Interface);
    USB_USBTask();
  }
}



void SetupHardware(void) {
  /* Disable watchdog if enabled by bootloader/fuses */
  MCUSR &= ~(1 << WDRF);
  wdt_disable();
  
  /* Disable clock division */
  clock_prescale_set(clock_div_1);
  
  /* Hardware Initialization */
  /* Set outputs to 1, other to 0 */
  DDRB = PORTB_OUTPUTS_MASK;
  
  /* Set pullup resistors on input pins */
  PORTB = (PORTB|PORTB_INPUTS_MASK);
  
  SerialDebug_init();
  USB_Init();
}

void EVENT_USB_Device_ConfigurationChanged(void) {
  bool ConfigSuccess = true;
  
  SerialDebug_printf("EVENT_USB_Device_ConfigurationChanged\r\n");
  ConfigSuccess &= HID_Device_ConfigureEndpoints(&Joystick_HID_Interface);
  
  USB_Device_EnableSOFEvents();
}

void EVENT_USB_Device_ControlRequest(void) {
  SerialDebug_printf("EVENT_USB_Device_ControlRequest\r\n");
  HID_Device_ProcessControlRequest(&Joystick_HID_Interface);
}

void EVENT_USB_Device_StartOfFrame(void) {
  //SerialDebug_printf("EVENT_USB_Device_StartOfFrame\r\n");
  HID_Device_MillisecondElapsed(&Joystick_HID_Interface);
}

bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
  USB_JoystickReport_Data_t* JoystickReport = (USB_JoystickReport_Data_t*)ReportData;
  
  portb_defs bport;
  
  /* Read inputs for select=high */
  bport.raw = PINB;
  set_select_to_low();
  
  JoystickReport->Y = 0;
  JoystickReport->X = 0;
  JoystickReport->button.raw[0] = 0;
  JoystickReport->button.raw[1] = 0;
  
  if (!bport.high.up)
    JoystickReport->Y = -127;
  else if (!bport.high.down)
    JoystickReport->Y =  127;
  
  if (!bport.high.left)
    JoystickReport->X = -127;
  else if (!bport.high.right)
    JoystickReport->X =  127;
  
  /* Read inputs for select=low */
  bport.raw = PINB;
  set_select_to_high();
  
  if (!bport.low.I_btn)
    JoystickReport->button.square = 1;
  
  if (!bport.low.II_btn)
    JoystickReport->button.cross = 1;
  
  if (!bport.low.select)
    JoystickReport->button.select = 1;
  
  if (!bport.low.run)
    JoystickReport->button.start = 1;
  
  *ReportSize = sizeof(USB_JoystickReport_Data_t);
  return false;
}

void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
  SerialDebug_printf("CALLBACK_HID_Device_ProcessHIDReport\r\n");
  SerialDebug_printf("%#0x, %#0x\r\n", ReportID, ReportType);
  SerialDebug_printByteArray(ReportData, ReportSize);
  
  // Unused (but mandatory for the HID class driver) in this demo, since there are no Host->Device reports
}
