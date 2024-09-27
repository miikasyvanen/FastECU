/**********************************************
*
* Arduino code to get MC68HC916Y5 to BDM mode
* and to upload kernel to read and write ECU
*
* Intended to use to recovery of bricked ECU
* or reverting locked ECU to stock.
*
* Usage: From serial terminal, i.e. Arduino serial monitor:
* > rpmem <addr_start> <block_length> hex
* This will print requested data as hex in single line to be able
* to copy/paste it to preferred hex editor and save as a bin file.
*
* On FastECU for reading: select MC68HC916Y5 BDM protocol, select used arduino board
* from serial port list, click read and wait ROM to download.
*
* On FastECU for writing: select MC68HC916Y5 BDM protocol, select used arduino board
* from serial port list, click write to upload MC68HC916Y5 kernel. After kernel upload,
* change to normal MC68HC916Y5 protocol, select used OBD adapter from serial port list,
* and click test write/write to write ROM to ECU.
*
**********************************************/

int DSO_pin = 2;        // PORTD pin1
int DSI_pin = 3;        // PORTD pin0
int RESET_pin = 4;      // PORTD pin4
int FREEZE_pin = 5;     // PORTC pin6
int BKPT_pin = 6;       // PORTD pin7
int DSCLK_pin = 6;      // PORTD pin7
int CLKOUT_pin = 7;     // PORTE pin6
int BERR_pin = 8;       // PORTB pin4
int DS_pin = 9;         // PORTB pin5
int MCU_RESET_pin = 10; // PORTB pin6

volatile uint8_t freeze_state = 0;
volatile int clkout = 0;
//unsigned long lastTime = 0;

uint8_t delay_clk = 2;

bool print_hex = false;
bool bdm_initialized = false;

// BDM commands
uint16_t CMD_RREGM = 0b0001011110000000;  // 0x1780 - Read contents of registers specified by command word register mask
uint16_t CMD_WREGM = 0b0001011110000001;  // 0x1781 - Write to registers specified by command word register mask
uint16_t CMD_RDMAC = 0b0001011110001010;  // 0x178A - Read contents of entire multiply and accumulate register set
uint16_t CMD_WRMAC = 0b0001011110001011;  // 0x178B - Write to entire multiply and accumulate register set
uint16_t CMD_RPCSP = 0b0001011110000010;  // 0x1782 - Read contents of program counter and stack pointer
uint16_t CMD_WPCSP = 0b0001011110000011;  // 0x1783 - Write to program counter and stack pointer
uint16_t CMD_RDMEM = 0b0001011110000100;  // 0x1784 - Read data from specified 20-bit address in data space
uint16_t CMD_WDMEM = 0b0001011110000101;  // 0x1785 - Write data to specified 20-bit address in data space
uint16_t CMD_RPMEM = 0b0001011110000110;  // 0x1786 - Read data from specified 20-bit address in program space
uint16_t CMD_WPMEM = 0b0001011110000111;  // 0x1787 - Write data to specified 20-bit address in program space
uint16_t CMD_GO = 0b0001011110001000;     // 0x1788 - Instruction pipeline flushed and refilled; instructions executed from current PC – $0006
uint16_t CMD_NOP = 0b0001011110001001;    // 0x1789 - Null command — performs no operation

// FEExCTL registers masks
uint16_t VFPE = 0x8;
uint16_t ERAS = 0x4;
uint16_t LAT = 0x2;
uint16_t ENPE = 0x1;

uint32_t FEE1CTL = 0xFF780; // 0xFF7D8
uint32_t FEE2CTL = 0xFF7C0; // 0xFF7E8
uint32_t FEE3CTL = 0xFF800; // 0xFF808
uint32_t FEE4CTL = 0xFF840; // 0xFF828
uint32_t FEE5CTL = 0xFF880; // 0xFF848

uint16_t reg_values[16];


void clkOutFunction()
{
  clkout++;
}

void wait_for_ready()
{
  while(digitalRead(DSO_pin) > 0)
  {
    digitalWrite(DSCLK_pin, HIGH);
    delayMicroseconds(delay_clk);
    digitalWrite(DSCLK_pin, LOW);
    delayMicroseconds(delay_clk);
  }
}

uint32_t transfer_word(uint16_t tx_word)
{
  uint32_t rx_word = 0;
  int data_bit = 0;
  bool error = false;

  digitalWrite(DSI_pin, LOW);
  digitalWrite(DSCLK_pin, LOW);
  rx_word |= digitalRead(DSO_pin);
  delayMicroseconds(delay_clk);
  digitalWrite(DSCLK_pin, HIGH);
  delayMicroseconds(delay_clk);

  for (int i = 0; i < 16; i++)
  {
    digitalWrite(DSI_pin, (tx_word & 0x8000) != 0);
    tx_word <<= 1;
    digitalWrite(DSCLK_pin, LOW);
    delayMicroseconds(delay_clk);
    rx_word = (rx_word << 1);
    rx_word |= digitalRead(DSO_pin);
    digitalWrite(DSCLK_pin, HIGH);
    delayMicroseconds(delay_clk);
  }
  //delayMicroseconds(1000);

  return rx_word;
}

void print_rom_data(uint8_t *rom_data)
{
  char str[3];
  char msg[128];

  memset(msg, 0, 128);
  
  if (!print_hex)
  {
    Serial.write(rom_data, 32);
  }
  else
  {
    for (int i = 0; i < 32; i++)
    {
      sprintf(str, "%02X", rom_data[i]);
      strcat(msg, str);
    }
    Serial.print(msg);
  }
}

uint32_t hex2dec(String hex_val) 
{ 
  int len = hex_val.length(); 
  uint32_t base = 1; 
  uint32_t dec_val = 0; 
  
  for (int i = len - 1; i >= 0; i--)
  { 
    if (hex_val[i] >= '0' && hex_val[i] <= '9')
    { 
      dec_val += (int(hex_val[i]) - 48) * base; 
      base = base * 16; 
    } 
    else if (hex_val[i] >= 'A' && hex_val[i] <= 'F')
    { 
      dec_val += (int(hex_val[i]) - 55) * base; 
      base = base * 16; 
    } 
    else if (hex_val[i] >= 'a' && hex_val[i] <= 'f')
    { 
      dec_val += (int(hex_val[i]) - 87) * base; 
      base = base * 16; 
    } 
  }
  return dec_val; 
}

void setup()
{
  Serial.begin(115200);

  pinMode(DS_pin, OUTPUT);
  digitalWrite(DS_pin, HIGH);
  pinMode(BERR_pin, INPUT);
  //digitalWrite(BERR_pin, HIGH);
  pinMode(CLKOUT_pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(CLKOUT_pin), clkOutFunction, RISING);
  pinMode(BKPT_pin, OUTPUT); // Also DSCLK_pin
  digitalWrite(BKPT_pin, HIGH);
  pinMode(FREEZE_pin, INPUT_PULLUP);
  pinMode(RESET_pin, OUTPUT);
  digitalWrite(RESET_pin, LOW);
  pinMode(DSI_pin, OUTPUT);
  digitalWrite(DSI_pin, LOW);
  pinMode(DSO_pin, INPUT_PULLUP);
  pinMode(MCU_RESET_pin, INPUT_PULLUP);
  
  //Serial.println("*** Welcome to MC68HC16Y5 unbrick tool! ***");
}

void loop()
{
  if (Serial.available())
  {
    char cmd[10] = { 0 };
    char operand_1[12] = { 0 };
    char operand_2[12] = { 0 };
    char operand_3[12] = { 0 };
    uint32_t data_addr = 0;
    uint32_t data_length = 0;
    uint32_t data_regs = 0;

    String str = Serial.readString();
    sscanf(str.c_str(), "%s %s %s %s", cmd, operand_1, operand_2, operand_3);
    //sscanf(str.c_str(), "%s", cmd);

    print_hex = false;
    if (strcmp(operand_1, "hex") == 0 || strcmp(operand_2, "hex") == 0 || strcmp(operand_3, "hex") == 0)
      print_hex = true;
    if (strcmp(operand_1, "HEX") == 0 || strcmp(operand_2, "HEX") == 0 || strcmp(operand_3, "HEX") == 0)
      print_hex = true;

    data_addr = hex2dec(operand_1);
    data_length = hex2dec(operand_2);
    data_regs = data_addr;
    if (!strcmp(cmd, "rpmem") || !strcmp(cmd, "rdmem"))
    {
      if (data_length < 32)
      {
        sprintf(operand_2, "0x20");
        data_length = 32;
      }
    }
    if (!strcmp(cmd, "init"))
    {
      initialize_bdm();
    }
    else if (!strcmp(cmd, "rpmem"))
    {
      //sscanf(str.c_str(), "%s %s %s %s", cmd, data_addr_hex, data_length_hex, data_type);
      read_memory(true, data_addr, data_length);
    }
    else if (!strcmp(cmd, "wpmem"))
    {
      //sscanf(str.c_str(), "%s %s %s", cmd, data_addr_hex, data_length_hex);
      Serial.print("ACK_CMD_WPMEM");
      write_memory(true, data_addr, data_length);
    }
    else if (!strcmp(cmd, "rdmem"))
    {
      //sscanf(str.c_str(), "%s %s %s %s", cmd, data_addr_hex, data_length_hex, data_type);
      read_memory(false, data_addr, data_length);
    }
    else if (!strcmp(cmd, "wdmem"))
    {
      //sscanf(str.c_str(), "%s %s %s", cmd, data_addr_hex, data_length_hex);
      Serial.print("ACK_CMD_WDMEM");
      write_memory(false, data_addr, data_length);
    }
    else if (!strcmp(cmd, "rpcsp"))
    {
      read_from_pc_sp();
    }
    else if (!strcmp(cmd, "wpcsp"))
    {
      write_to_pc_sp();
    }
    else if (!strcmp(cmd, "go"))
    {
      exit_bdm_mode();
    }
    else if (!strcmp(cmd, "erase"))
    {
      //sscanf(str.c_str(), "%s %s %s", cmd, data_regs_hex, data_type);
      erase_memory();
    }
    else if (!strcmp(cmd, "rreg"))
    {
      read_regs_with_mask(data_regs);
    }
    else if (!strcmp(cmd, "wreg"))
    {
      write_regs_with_mask(data_regs);
    }
    else if (!strcmp(cmd, "stop"))
    {
      Serial.println("Stop BDM operation");
    }
    else
    {
      Serial.print("ERROR! Unknown command: ");
      Serial.print("'");
      Serial.print(cmd);
      Serial.println("'");
    }
  }

  freeze_state = digitalRead(FREEZE_pin);
  if(!freeze_state)
    bdm_initialized = false;

}

int initialize_bdm()
{
  pinMode(DS_pin, OUTPUT);
  digitalWrite(DS_pin, HIGH);
  pinMode(BERR_pin, INPUT);
  pinMode(CLKOUT_pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(CLKOUT_pin), clkOutFunction, RISING);
  pinMode(BKPT_pin, OUTPUT); // Also DSCLK_pin
  digitalWrite(BKPT_pin, HIGH);
  pinMode(FREEZE_pin, INPUT_PULLUP);
  pinMode(RESET_pin, OUTPUT);
  digitalWrite(RESET_pin, LOW);
  pinMode(DSI_pin, OUTPUT);
  digitalWrite(DSI_pin, LOW);
  pinMode(DSO_pin, INPUT_PULLUP);
  pinMode(MCU_RESET_pin, INPUT_PULLUP);
  delay(100);

  digitalWrite(BKPT_pin, LOW);
  digitalWrite(RESET_pin, LOW);
  delay(50);
  //Serial.println(freeze_state, DEC);
  digitalWrite(RESET_pin, HIGH);
  delayMicroseconds(20);
  
  /* digitalRead(MCU_RESET_pin) */
  asm (
    "in __tmp_reg__, __SREG__  \n"
    "cli                       \n" //disable interrupts
    "1: sbis %0, %1            \n" //skip next if pin high
    "rjmp 1b                   \n"
    "out __SREG__, __tmp_reg__ \n"
    :: "I" (_SFR_IO_ADDR(PINB)), "I" (PINB6)
  );
  
  /* digitalWrite(BKPT_pin, HIGH); */
  asm volatile(
    "SBI %0, %1 \n\t"
    :: "I" (_SFR_IO_ADDR(PORTD)), "I" (PORTD7)
  );
  /* freeze_state = digitalRead(FREEZE_pin); */
  asm (
    "in __tmp_reg__, __SREG__  \n"
    "cli                       \n" //disable interrupts
    "ldi %0, 1                 \n"
    "1: sbis %1, %2            \n" //skip next if pin high
    "rjmp 1b                   \n"
    "out __SREG__, __tmp_reg__ \n"
    : "=a" (freeze_state) : "I" (_SFR_IO_ADDR(PINB)), "I" (PINB6)
  );
  /* digitalWrite(DSI_pin, LOW); */
  asm volatile(
    "CBI %0, %1 \n\t"
    :: "I" (_SFR_IO_ADDR(PORTD)), "I" (PORTD0)
  );
  delay(40);

  return 0;
}

int exit_bdm_mode()
{
  uint32_t response = 0;
  uint8_t rom_data[32] = { 0 };

  if (!bdm_initialized)
    if (initialize_bdm())
      return 1;
  bdm_initialized = true;
  
  response = transfer_word(CMD_GO);
  //response = transfer_word(CMD_NOP);
  Serial.print("Exiting BDM mode! Response: 0x");
  rom_data[0] = (response >> 8) & 0xFF;
  rom_data[1] = response & 0xFF;
  print_rom_data(rom_data);
  Serial.println("");

  pinMode(DS_pin, INPUT);
  pinMode(BERR_pin, INPUT);
  pinMode(CLKOUT_pin, INPUT);
  pinMode(BKPT_pin, INPUT); // Also DSCLK_pin
  pinMode(FREEZE_pin, INPUT);
  //pinMode(RESET_pin, INPUT);
  pinMode(DSI_pin, INPUT);
  pinMode(DSO_pin, INPUT);
  pinMode(MCU_RESET_pin, INPUT);

  return 0;
}

int read_memory(bool read_rom, uint32_t data_addr, uint32_t data_length)
{
  uint32_t addr = 0;
  uint32_t start_addr = data_addr;
  uint32_t end_addr = data_addr + data_length;
  uint32_t addr_bank = 0;
  uint32_t response1 = 0;
  uint32_t response2 = 0;
  uint32_t response3 = 0;
  uint32_t response4 = 0;
  uint16_t cmd = 0;
  uint8_t rom_data[32] = { 0 };
  uint8_t rom_data_index = 0;
  bool initial_req = true;

  if (!bdm_initialized)
    if (initialize_bdm())
      return 1;
  bdm_initialized = true;

  if (read_rom)
    cmd = CMD_RPMEM;
  else
  {
    cmd = CMD_RDMEM;
    start_addr |= 0x40000000;
    end_addr |= 0x40000000;
  }
    
  for (addr = start_addr; addr < end_addr; addr += 2)
  {
    response1 = transfer_word(cmd);
    response2 = transfer_word((addr >> 16) & 0xFFFF);
    response3 = transfer_word(addr & 0xFFFF);

    if (!initial_req)
    {
      rom_data[rom_data_index] = (response1 >> 8) & 0xFF;
      rom_data[rom_data_index + 1] = response1 & 0xFF;
      rom_data_index+=2;
      if (rom_data_index > 31)
      {
        print_rom_data(rom_data);
        rom_data_index = 0;
      }
    }
    initial_req = false;
  }
  response1 = transfer_word(CMD_NOP);
  rom_data[rom_data_index] = (response1 >> 8) & 0xFF;
  rom_data[rom_data_index + 1] = response1 & 0xFF;
  print_rom_data(rom_data);

  Serial.println("");
  return 0;
}

int write_memory(bool write_rom, uint32_t data_addr, uint32_t data_length)
{
  uint32_t addr = 0;
  uint32_t start_addr = data_addr;
  uint32_t end_addr = data_addr + data_length;
  uint32_t addr_bank = 0;
  uint32_t response = 0;
  uint32_t loopcount = 0;
  uint32_t byte_addr = 0;
  uint16_t cmd = 0;
  uint16_t data = 0;
  uint8_t packet_length = 0;
  uint8_t rom_data[32] = { 0 };
  uint8_t rom_data_index = 0;
  bool initial_req = true;

  if (!bdm_initialized)
    if (initialize_bdm())
      return 1;
  bdm_initialized = true;

  if (write_rom)
    cmd = CMD_WPMEM;
  else
  {
    cmd = CMD_WDMEM;
    start_addr |= 0x40000000;
    end_addr |= 0x40000000;
  }

  if ((end_addr - start_addr) > 31)
    packet_length = 32;
  else
    packet_length = data_length;

  for (addr = start_addr; addr < end_addr; addr += packet_length)
  {
    rom_data_index = 0;
    byte_addr = addr;

    if ((end_addr - addr) > 31)
      packet_length = 32;
    else
      packet_length = data_length;
      
    while(rom_data_index < packet_length)
    {
      if(Serial.available())
      {
        rom_data[rom_data_index] = Serial.read();
        rom_data_index++;
      }
      delay(1);
    }
    for (int i = 0; i < packet_length; i+=2)
    {
      response = 0;
      loopcount = 0;
      data = (rom_data[i] << 8) + rom_data[i + 1];
      
      if (!initial_req)
      {
        response = transfer_word(cmd);
        while(response != 0xFFFF && loopcount < 2000)
        {
          response = transfer_word(cmd);
          loopcount++;
          delay(2);
        }
      }
      else
        response = transfer_word(cmd);
        
      if (loopcount < 2000)
      {
        response = transfer_word(((addr + i) >> 16) & 0xFFFF);
        response = transfer_word((addr + i) & 0xFFFF);
        response = transfer_word(data);
        //addr+=2;
      }
      else
      {
        Serial.println("Failed to get response from ECU");
        //break;
      }
      initial_req = false;
    }
    Serial.print("ACK_WR");
  }
  loopcount = 0;
  response = transfer_word(CMD_NOP);
  while(response != 0xFFFF && loopcount < 2000)
  {
    response = transfer_word(CMD_NOP);
    loopcount++;
    delay(2);
  }
  if (loopcount < 2000)
    Serial.print("ACK_WR");
  else
    Serial.println("Failed to get response from ECU");

  return 0;
}

uint32_t read_from_pc_sp()
{
  uint32_t response = 0;
  uint8_t rom_data[32] = { 0 };

  if (!bdm_initialized)
    if (initialize_bdm())
      return 1;
  bdm_initialized = true;

  response = transfer_word(CMD_RPCSP);
  for (int i = 0; i < 4; i++)
  {
    response = transfer_word(CMD_NOP);
    rom_data[i*2] = (response >> 8) & 0xFF;
    rom_data[i*2+1] = response & 0xFF;
  }
  Serial.print("Read PC and SP finished! Response: 0x");
  print_rom_data(rom_data);
  Serial.println("");

  return rom_data;
}

int write_to_pc_sp()
{
  uint32_t response = 0;
  uint32_t *pc_sp;
  //uint8_t pc_sp[32];
  uint8_t rom_data[32] = { 0 };

  if (!bdm_initialized)
    if (initialize_bdm())
      return 1;
  bdm_initialized = true;

  pc_sp = read_from_pc_sp();
  print_rom_data((uint8_t)pc_sp);
  Serial.println("");

  uint16_t pk_reg = ((pc_sp[0] << 8) & 0xFF) + (pc_sp[1] & 0xFF);
  uint16_t pc_reg = ((pc_sp[2] << 8) & 0xFF) + (pc_sp[3] & 0xFF);
  uint16_t sk_reg = ((pc_sp[4] << 8) & 0xFF) + (pc_sp[5] & 0xFF);
  uint16_t sp_reg = ((pc_sp[6] << 8) & 0xFF) + (pc_sp[7] & 0xFF);
  
  response = transfer_word(CMD_WPCSP);
  response = transfer_word(0x0002);
  response = transfer_word(0x0006);
  response = transfer_word(0x0002);
  response = transfer_word(0x0878);
  response = transfer_word(CMD_NOP);
  Serial.print("Write PC and SP finished! Response: 0x");
  rom_data[0] = (response >> 8) & 0xFF;
  rom_data[1] = response & 0xFF;
  print_rom_data(rom_data);
  Serial.println("");
  read_from_pc_sp();

  return 0;
}

int erase_memory()
{
  uint32_t addr = 0x0;
  uint32_t FEExBASE = 0xFF780;
  //uint32_t FEExBASE = 0xFF7C0;
  //uint32_t FEExBASE = 0xFF800;
  uint32_t FEExGAP = 0x40;
  uint32_t FEE1CTL = FEExBASE + 9;
  uint32_t FEExCTL = 0;
  uint32_t FEExADDR = 0;
  uint32_t response = 0;
  uint16_t cmd;
  uint16_t data = 0;
  uint8_t rom_data[32] = { 0 };

  uint16_t Tpr = 1;     // Program Recovery Time (usecs)
  uint16_t PWpp = 25;   // Program Pulse Width (usecs)
  uint8_t Npp = 50;     // Number of Program Pulses
  uint8_t Pm = 0;       // Program Margin
  uint8_t Nep = 5;      // Number of Erase Pulses
  uint16_t Tepk = 110;  // Erase Pulse Time(tei × k)(ms)
  uint16_t Tei = 110;   // Amount to Increment (ms)
  uint8_t Ter = 1;      // Erase Recovery Time (ms)
  
  bool flash_erase_ok = false;

  // VFPE     0x08
  // ERAS     0x04
  // LAT      0x02
  // ENPE     0x01

  //uint32_t FEE1CTL = 0xFF000 + 0xC2B;

  if (!bdm_initialized)
    if (initialize_bdm())
      return 1;
  bdm_initialized = true;

  cmd = CMD_WDMEM;
  FEExADDR = FEExBASE;// | 0x40000000;
  FEExCTL = FEE1CTL;// | 0x40000000;
  print_hex = true;
  
  for (int i = 0; i < 3; i++)
  {
    Serial.print("Erasing block 0x0");
    Serial.print(i, HEX);
    Serial.println(", please wait...");

    if (i == 0)
      addr = 0x00000010;
    if (i > 2)
      addr = 0x00010010;
    if (i > 3)
      addr = 0x00028010;


    Serial.print("Check FEExMCR at address 0x");
    Serial.print(FEExADDR, HEX);
    Serial.print(": ");
    read_memory(false, FEExADDR, 0x20);
    Serial.println("");

    Serial.print("Check FEExMCR (~LOCK) at address 0x");
    Serial.print(FEExADDR, HEX);
    Serial.print(": ");
    read_memory(false, FEExADDR, 0x20);
    Serial.println("");

    Serial.print("Check FEExMCR (FRZ) at address 0x");
    Serial.print(FEExADDR, HEX);
    Serial.print(": ");
    read_memory(false, FEExADDR, 0x20);
    Serial.println("");

    data |= ERAS;
    data |= LAT;
    response = transfer_word(cmd);
    response = transfer_word((FEExCTL >> 16) & 0xFFFF);
    response = transfer_word(FEExCTL & 0xFFFF);
    response = transfer_word(data);

    Serial.print("Check ERAS LAT at address 0x");
    Serial.print(FEExADDR, HEX);
    Serial.print(": ");
    read_memory(false, FEExADDR, 0x20);
    Serial.println("");
    Serial.println("------------------------------------------------------------------------------------------");

    flash_erase_ok = false;

    for (int loop = 1; loop <= Nep; loop++)
    {
      Tepk = Tei * loop;
      
      response = transfer_word(CMD_WPMEM);
      response = transfer_word((addr >> 16) & 0xFFFF);
      response = transfer_word(addr & 0xFFFF);
      response = transfer_word(data);
      delay(1);

      Serial.print("Check WPMEM at address 0x");
      Serial.print(FEExADDR, HEX);
      Serial.print(": ");
      read_memory(false, FEExADDR, 0x20);
      Serial.println("");

      data |= ENPE;
      response = transfer_word(cmd);
      response = transfer_word((FEExCTL >> 16) & 0xFFFF);
      response = transfer_word(FEExCTL & 0xFFFF);
      response = transfer_word(data);
      delay(Tepk);

      Serial.print("Check ENPE at address 0x");
      Serial.print(FEExADDR, HEX);
      Serial.print(": ");
      read_memory(false, FEExADDR, 0x20);
      Serial.println("");

      data &= ~ENPE;
      response = transfer_word(cmd);
      response = transfer_word((FEExCTL >> 16) & 0xFFFF);
      response = transfer_word(FEExCTL & 0xFFFF);
      response = transfer_word(data);
      delay(Ter);

      Serial.print("Check ~ENPE at address 0x");
      Serial.print(FEExADDR, HEX);
      Serial.print(": ");
      read_memory(false, FEExADDR, 0x20);
      Serial.println("");

      Serial.print("Check FEExMCR (~STOP) at address 0x");
      Serial.print(FEExADDR, HEX);
      Serial.print(": ");
      read_memory(false, FEExADDR, 0x20);
      Serial.println("");

      response = transfer_word(CMD_RPMEM);
      response = transfer_word((addr >> 16) & 0xFFFF);
      response = transfer_word(addr & 0xFFFF);
      response = transfer_word(CMD_NOP);
      if (response == 0xFFFF)
      {
        Serial.print("Try: ");
        Serial.print(loop, DEC);
        Serial.print(" - response: 0x");
        read_memory(true, addr, 0x20);
        Serial.println("");
        flash_erase_ok = true;
        break;
      }
      else
      {
        Serial.print("Try: ");
        Serial.print(loop, DEC);
        Serial.print(" - response: 0x");
        read_memory(true, addr, 0x20);
        Serial.println("");
      }
      Serial.println("------------------------------------------------------------------------------------------");
      delay(1);
    }
    data &= ~ERAS;
    data &= ~LAT;
    response = transfer_word(cmd);
    response = transfer_word((FEExCTL >> 16) & 0xFFFF);
    response = transfer_word(FEExCTL & 0xFFFF);
    response = transfer_word(data);

    if (flash_erase_ok)
    {
      Serial.print("Erase of block 0x0");
      Serial.print(i, HEX);
      Serial.println(" complete");
    }
    else
    {
      Serial.print("Erase of block 0x0");
      Serial.print(i, HEX);
      Serial.println(" FAILED!");
      break;
    }

    FEExCTL += FEExGAP;
    FEExADDR += FEExGAP;
  }  
  return 0;
}

int read_regs_with_mask(int mask)
{
  //uint32_t start_addr = data_addr;
  uint32_t reg_addr = 0xFF7D8;
  uint32_t response1 = 0;
  uint32_t response2 = 0;
  uint8_t rom_data[32] = { 0 };
  uint8_t rom_data_index = 0;

  if (!bdm_initialized)
    if (initialize_bdm())
      return 1;
  bdm_initialized = true;

  Serial.println("");
  Serial.print("Read registers from mask: 0x");
  Serial.println(mask, HEX);

  response1 = transfer_word(CMD_RREGM);
  response2 = transfer_word(mask & 0xFFFF);
  for (int i = 0; i < 7; i++)
  {
    response1 = transfer_word(CMD_NOP);
    rom_data[rom_data_index] = (response1 >> 8) & 0xFF;
    rom_data[rom_data_index + 1] = response1 & 0xFF;
    rom_data_index+=2;
    reg_values[i] = response1;
  }
  print_rom_data(rom_data);

  Serial.println("");
  Serial.println("Registers read complete");
  
  return 0;
}

int write_regs_with_mask(int mask)
{
  uint32_t response1 = 0;
  uint32_t response2 = 0;

  reg_values[0] = 0x48E0;
  reg_values[1] = 0x22F2;
  reg_values[2] = 0x0000;
  reg_values[3] = 0xFA00;
  reg_values[4] = 0x85DC;
  reg_values[5] = 0x330B;
  reg_values[6] = 0xFF0B;
  reg_values[7] = 0x0000;

  if (!bdm_initialized)
    if (initialize_bdm())
      return 1;
  bdm_initialized = true;

  Serial.println("");
  Serial.print("Write registers from mask: 0x");
  Serial.println(mask, HEX);

  response1 = transfer_word(CMD_WREGM);
  response2 = transfer_word(mask & 0xFFFF);
  for (int i = 0; i < 7; i++)
  {
    response1 = transfer_word(reg_values[i] & 0xFFFF);
  }
  
  return 0;
}
