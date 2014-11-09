
#include "Joystick.h"

static uint8_t PrevJoystickHIDReportBuffer[sizeof(USB_JoystickReport_Data_t)];

/* TODO: Need to check at which frequency the console is toggling
         the chip_enable pin to reproduce the same autofire frequency.
static bool JoystickNeedsReset;

ISR(TIMER1_COMPA_vect) {
  JoystickNeedsReset = true;
}

void init_reset_timer() {
  JoystickNeedsReset = false;
  TCCR1A = 0;
  TCCR1B = _BV(WGM12);
  TCCR1C = 0;
  TCNT1 = 0;
  OCR1A = F_CPU/60;
  TIMSK1 |= _BV(OCIE1A); // enable timer interruption
  TCCR1B |= _BV(CS10); // enable timer clock
}
*/

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
  } directions;
  struct {
    unsigned int I_btn:1;
    unsigned int II_btn:1;
    unsigned int select:1;
    unsigned int run:1;
    
    unsigned int pad:4;
  } btn1;
  struct {
    unsigned int III_btn:1;
    unsigned int IV_btn:1;
    unsigned int V_btn:1;
    unsigned int VI_btn:1;
    
    unsigned int pad:4;
  } btn2;
} portb_defs;

int main(void) {
  SetupHardware();
  
  GlobalInterruptEnable();
  
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
  //init_reset_timer();
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

/* if all directions are pressed (0), it is not a direction information */
inline bool is_direction(portb_defs bport) {
  return (bport.directions.up|
	  bport.directions.down|
	  bport.directions.left|
	  bport.directions.right);
}

inline void SetDirections(USB_JoystickReport_Data_t* JoystickReport, portb_defs bport) {
    if (!bport.directions.up)
    JoystickReport->Y = -127;
  else if (!bport.directions.down)
    JoystickReport->Y =  127;
  
  if (!bport.directions.left)
    JoystickReport->X = -127;
  else if (!bport.directions.right)
    JoystickReport->X =  127;
}

inline void SetButtons1(USB_JoystickReport_Data_t* JoystickReport, portb_defs bport) {
  if (!bport.btn1.I_btn)
    JoystickReport->button.square = 1;
  
  if (!bport.btn1.II_btn)
    JoystickReport->button.cross = 1;
  
  if (!bport.btn1.select)
    JoystickReport->button.select = 1;
  
  if (!bport.btn1.run)
    JoystickReport->button.start = 1;
}

inline void SetButtons2(USB_JoystickReport_Data_t* JoystickReport, portb_defs bport) {
  if (!bport.btn2.III_btn)
    JoystickReport->button.circle = 1;
  
  if (!bport.btn2.IV_btn)
    JoystickReport->button.triangle = 1;
  
  if (!bport.btn2.V_btn)
    JoystickReport->button.l1 = 1;
  
  if (!bport.btn2.VI_btn)
    JoystickReport->button.r1 = 1;
}

inline void ReadController(USB_JoystickReport_Data_t* JoystickReport) {
  portb_defs bport;
  bport.raw = PINB;
  set_select_to_low();
  if (is_direction(bport)) {
    SetDirections(JoystickReport, bport);
    bport.raw = PINB;
    set_select_to_high();
    SetButtons1(JoystickReport, bport);
  } else {
    bport.raw = PINB;
    set_select_to_high();
    SetButtons2(JoystickReport, bport);
  }
}

inline void UpdateControllerStatus(USB_JoystickReport_Data_t* JoystickReport) {
  enable_controller_chip();
  set_select_to_high();
  
  JoystickReport->Y = 0;
  JoystickReport->X = 0;
  JoystickReport->button.raw[0] = 0;
  JoystickReport->button.raw[1] = 0;
  
  ReadController(JoystickReport);
  ReadController(JoystickReport);
  
  /*if (JoystickNeedsReset) {
    JoystickNeedsReset = false;
    disable_controller_chip();
    }*/
  disable_controller_chip();
}

bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
  USB_JoystickReport_Data_t* JoystickReport = (USB_JoystickReport_Data_t*)ReportData;
  UpdateControllerStatus(JoystickReport);
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
