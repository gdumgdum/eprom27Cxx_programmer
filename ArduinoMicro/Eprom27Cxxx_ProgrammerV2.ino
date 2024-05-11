/* For Arduino Micro */

/* 74HC595 control (address lines) */
#define shiftLatchPin 14
#define shiftClockPin 23
#define shiftDataPin 15

/* Data pins */
#define dataB0 4
#define dataB1 3
#define dataB2 2
#define dataB3 5
#define dataB4 6
#define dataB5 7
#define dataB6 8
#define dataB7 9

/* Chip control */
#define pinCE 16
#define pinOE 17
#define pinProgramC1001C2001 20 // For 27C1001 and 27C2001
#define powerEnableC16C32 10 // For 27C16 and 27C32
#define powerEnableC64C128C256C512 18 // For 27C64, 27C128, 27C256 and 27C512
#define programVoltageEnableC16 11 // For 27C16
#define programVoltageEnableC32C512C801 12 // For 27C32, 27C512 and 27C801
#define programVoltageEnableC64C128C256 13 // For 27C64, 27C128 and 27C256
#define programVoltageEnableC1001C2001C4001 21 // For 27C1001, 27C2001 and 27C4001

#define SerialSpeed 115200

//#define MESSAGES

typedef enum chipType {
  NONE = 0,
  C16 = 1,
  C32 = 2,
  C64 = 3,
  C128 = 4,
  C256 = 5,
  C512 = 6,
  C1001 = 7,
  C2001 = 8,
  C4001 = 9,
  C801 = 10
} Chip;

typedef enum mode {
  WAIT,
  READ,
  WRITE,
  VOLTAGE
} Modes;


void write_mode (void);
void read_mode (void);
void set_address (uint32_t address);
uint8_t get_data (void);
void set_data (uint8_t data);
uint8_t read_byte (uint32_t address);
void write_byte (uint32_t address, uint8_t data);
float get_voltage (void);
void program_voltage_set (bool state);
uint32_t gen_address (uint32_t address);
void select_chip (chipType new_chip);

/* Voltage control (for programming chips) */
#define voltageControl A1
#define rTop 10000.0
#define rBottom 1500.0

chipType chip = NONE;
Modes mode = WAIT;
uint32_t start_address = 0x00000000;
uint32_t end_address = 0x00000000;
#define BUF_LEN 32
uint8_t buf[BUF_LEN];

void setup() {
  // 74HC595 (*3)
  pinMode(shiftLatchPin, OUTPUT);
  pinMode(shiftClockPin, OUTPUT);
  pinMode(shiftDataPin, OUTPUT);

  // Chip control
  pinMode(pinCE, OUTPUT);
  pinMode(pinOE, OUTPUT);
  pinMode(pinProgramC1001C2001, OUTPUT);
  pinMode(powerEnableC16C32, OUTPUT);
  pinMode(powerEnableC64C128C256C512, OUTPUT);
  pinMode(programVoltageEnableC16, OUTPUT);
  pinMode(programVoltageEnableC32C512C801, OUTPUT);
  pinMode(programVoltageEnableC64C128C256, OUTPUT);
  pinMode(programVoltageEnableC1001C2001C4001, OUTPUT);
  digitalWrite(pinOE, HIGH);
  digitalWrite(pinProgramC1001C2001, HIGH);
  digitalWrite(powerEnableC16C32, HIGH);
  digitalWrite(powerEnableC64C128C256C512, HIGH);
  digitalWrite(programVoltageEnableC16, LOW);
  digitalWrite(programVoltageEnableC32C512C801, LOW);
  digitalWrite(programVoltageEnableC64C128C256, LOW);
  digitalWrite(programVoltageEnableC1001C2001C4001, LOW);
  digitalWrite(pinCE, LOW);

  // Data pins
  read_mode();

  Serial.begin(SerialSpeed);
  Serial.setTimeout(2000);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB on Micro
  }
}

void loop() {
  switch (mode) {
    case READ:
      if (chip == NONE) {
        mode = WAIT;
        break;
      }
#ifdef MESSAGES
      Serial.println("Read mode.");
#endif
      read_mode();
      digitalWrite(pinCE, LOW);
      digitalWrite(pinOE, LOW);
      delay(10); // let time for voltages to stabilize
      for (uint32_t i = start_address; i <= end_address; i++) {
        uint8_t data = read_byte(i);
        Serial.write(&data, sizeof(data));
      }
      digitalWrite(pinOE, HIGH);
      digitalWrite(pinCE, HIGH);
      Serial.flush();
      mode = WAIT;
      break;
    case WRITE:
      if (chip == NONE) {
        mode = WAIT;
        break;
      }
#ifdef MESSAGES
      Serial.print("n");
#endif
      write_mode();
      program_voltage_set(true);
      delay(10); // let time for voltages to stabilize

      for (uint32_t i = start_address; i <= end_address; i += BUF_LEN) {
        Serial.println("Waiting for data");
        Serial.flush();

        do { } while (Serial.available() < BUF_LEN); // wait for at least BUF_LEN characters in the serial read buffer

        uint8_t count = Serial.readBytes(buf, BUF_LEN);
        if (count != BUF_LEN) {
          Serial.print("Error on block ");
          Serial.println(i / BUF_LEN);
          Serial.print("Received ");
          Serial.println(count);
          Serial.flush();
          break;
        }

        Serial.print("Write block ");
        Serial.println(i / BUF_LEN);
        Serial.flush();

        for (int j = 0; j < BUF_LEN; j++) {
          write_byte(i + j, buf[j]);
        }
      }
      program_voltage_set(false);

      Serial.println("Programming Done");
      Serial.flush();

#ifdef MESSAGES
      Serial.println("Write success.");
#endif
      mode = WAIT;
      break;
    case VOLTAGE:
      Serial.print("Programming voltage: ");
      Serial.println(get_voltage(), 1);
      Serial.flush();
      mode = WAIT;
      break;
    default:
#ifdef MESSAGES
      if (chip == NONE) Serial.println("Chip not selected!");
      Serial.println("Wait commands...");
#endif
      while (Serial.available()) Serial.read();
      do {} while (Serial.available() == 0); // wait for at least 1 character in the serial read buffer
      char incomingByte = Serial.read();      // read the first (and hopefully only) character
      while (Serial.available()) Serial.read(); // read remaining data, if any
      switch (incomingByte) {
        case 'x': 
          chip = NONE;
          start_address = 0x00000000;
          end_address = 0x00000000;
          Serial.println("Arduino 27 Series programmer V2");
          Serial.flush();
          break;
        case 'r': mode = READ; break;
        case 'w': mode = WRITE; break;
        case 'v': mode = VOLTAGE; break;
        case 'a': select_chip(C16); break;
        case 'b': select_chip(C32); break;
        case 'c': select_chip(C64); break;
        case 'd': select_chip(C128); break;
        case 'e': select_chip(C256); break;
        case 'f': select_chip(C512); break;
        case 'g': select_chip(C1001); break;
        case 'h': select_chip(C2001); break;
        case 'i': select_chip(C4001); break;
        case 'j': select_chip(C801); break;
      }
  }
}

void select_chip (chipType new_chip) {
  switch (new_chip) {
    case C16:
      digitalWrite(powerEnableC64C128C256C512, HIGH);
      digitalWrite(pinProgramC1001C2001, HIGH);
      digitalWrite(powerEnableC16C32, LOW);
      chip = new_chip;
      end_address = 0x000007ff;
#ifdef MESSAGES
      Serial.println("Select 27C16 chip.");
#endif
      break;
    case C32:
      digitalWrite(powerEnableC64C128C256C512, HIGH);
      digitalWrite(pinProgramC1001C2001, HIGH);
      digitalWrite(powerEnableC16C32, LOW);
      chip = new_chip;
      end_address = 0x00000fff;
#ifdef MESSAGES
      Serial.println("Select 27C32 chip.");
#endif
      break;
    case C64:
      digitalWrite(powerEnableC16C32, HIGH);
      digitalWrite(pinProgramC1001C2001, HIGH);
      digitalWrite(powerEnableC64C128C256C512, LOW);
      chip = new_chip;
      end_address = 0x00001fff;
#ifdef MESSAGES
      Serial.println("Select 27C64 chip.");
#endif
      break;
    case C128:
      digitalWrite(powerEnableC16C32, HIGH);
      digitalWrite(pinProgramC1001C2001, HIGH);
      digitalWrite(powerEnableC64C128C256C512, LOW);
      chip = new_chip;
      end_address = 0x00003fff;
#ifdef MESSAGES
      Serial.println("Select 27C128 chip.");
#endif
      break;
    case C256:
      digitalWrite(powerEnableC16C32, HIGH);
      digitalWrite(pinProgramC1001C2001, HIGH);
      digitalWrite(powerEnableC64C128C256C512, LOW);
      chip = new_chip;
      end_address = 0x00007fff;
#ifdef MESSAGES
      Serial.println("Select 27C256 chip.");
#endif
      break;
    case C512:
      digitalWrite(powerEnableC16C32, HIGH);
      digitalWrite(pinProgramC1001C2001, HIGH);
      digitalWrite(powerEnableC64C128C256C512, LOW);
      chip = C512;
      end_address = 0x0000ffff;
#ifdef MESSAGES
      Serial.println("Select 27C512 chip.");
#endif
      break;
    case C1001:
      digitalWrite(powerEnableC16C32, HIGH);
      digitalWrite(powerEnableC64C128C256C512, HIGH);
      digitalWrite(pinProgramC1001C2001, LOW);
      chip = C1001;
      end_address = 0x0001ffff;
#ifdef MESSAGES
      Serial.println("Select 27C1001 chip.");
#endif
      break;
    case C2001:
      digitalWrite(powerEnableC16C32, HIGH);
      digitalWrite(powerEnableC64C128C256C512, HIGH);
      digitalWrite(pinProgramC1001C2001, LOW);
      chip = C2001;
      end_address = 0x0003ffff;
#ifdef MESSAGES
      Serial.println("Select 27C2001 chip.");
#endif
      break;
    case C4001:
      digitalWrite(powerEnableC16C32, HIGH);
      digitalWrite(powerEnableC64C128C256C512, HIGH);
      digitalWrite(pinProgramC1001C2001, HIGH);
      chip = C4001;
      end_address = 0x0007ffff;
#ifdef MESSAGES
      Serial.println("Select 27C4001 chip.");
#endif
      break;
    case C801:
      digitalWrite(powerEnableC16C32, HIGH);
      digitalWrite(powerEnableC64C128C256C512, HIGH);
      digitalWrite(pinProgramC1001C2001, HIGH);
      chip = C801;
      end_address = 0x000fffff;
#ifdef MESSAGES
      Serial.println("Select 27C801 chip.");
#endif
      break;
    default:
      chip = NONE;
      end_address = 0x00000000;
#ifdef MESSAGES
      Serial.println("Chip not selected!");
#endif
  }
}


void program_voltage_set (bool state) {
  switch (chip) {
    case C16:
      digitalWrite(programVoltageEnableC16, state);
      break;
    case C32:
    case C512:
    case C801:
      digitalWrite(programVoltageEnableC32C512C801, state);
      break;
    case C64:
    case C128:
    case C256:
      digitalWrite(programVoltageEnableC64C128C256, state);
      break;
    case C1001:
    case C2001:
    case C4001:
      digitalWrite(programVoltageEnableC1001C2001C4001, state);
      break;
    default:
#ifdef MESSAGES
      Serial.println("Chip not selected!");
#endif
      break;
  }
}


void write_mode (void) {
  pinMode(dataB0, OUTPUT);
  pinMode(dataB1, OUTPUT);
  pinMode(dataB2, OUTPUT);
  pinMode(dataB3, OUTPUT);
  pinMode(dataB4, OUTPUT);
  pinMode(dataB5, OUTPUT);
  pinMode(dataB6, OUTPUT);
  pinMode(dataB7, OUTPUT);
}

void read_mode (void) {
  pinMode(dataB0, INPUT_PULLUP);
  pinMode(dataB1, INPUT_PULLUP);
  pinMode(dataB2, INPUT_PULLUP);
  pinMode(dataB3, INPUT_PULLUP);
  pinMode(dataB4, INPUT_PULLUP);
  pinMode(dataB5, INPUT_PULLUP);
  pinMode(dataB6, INPUT_PULLUP);
  pinMode(dataB7, INPUT_PULLUP);
}

uint32_t gen_address (uint32_t address) {
  switch (chip) {
    case C16:
      break;
      if (mode == READ) {
        address |= 0x00000800;
        // high |= 1 << 3; // A11 (C32+) is Vpp for C16 (5v for read)
      }
      break;
    case C64:
    case C128:
      if (mode == READ) {
        address |= 0x00004000;
        // high |= 1 << 6; // A14 (C256+) is ~PGM for C64 and C128
      }
      break;
    case C32:
    case C256:
    case C512:
      break;
    case C1001:
    case C2001:
      if (mode == READ) {
        address |= 0x00040000; // A18 (C4001 and C801) is ~P for C1001 and C2001
      }
      break;
    case C4001:
    case C801:
    default:
      break;
  }
  return address;
}

void set_address (uint32_t address) {
  address = gen_address(address);
  digitalWrite(shiftLatchPin, LOW);
  uint16_t word = address; 
  byte registerThree = lowByte((address & 0x00ff0000) >> 16);
  byte registerTwo = highByte(word);
  byte registerOne = lowByte(word);
  shiftOut(shiftDataPin, shiftClockPin, MSBFIRST, registerThree);
  shiftOut(shiftDataPin, shiftClockPin, MSBFIRST, registerTwo);
  shiftOut(shiftDataPin, shiftClockPin, MSBFIRST, registerOne);
  digitalWrite(shiftLatchPin, HIGH);
}

uint8_t get_data (void) {
  uint8_t data = 0;
  data |= digitalRead(dataB0) << 0;
  data |= digitalRead(dataB1) << 1;
  data |= digitalRead(dataB2) << 2;
  data |= digitalRead(dataB3) << 3;
  data |= digitalRead(dataB4) << 4;
  data |= digitalRead(dataB5) << 5;
  data |= digitalRead(dataB6) << 6;
  data |= digitalRead(dataB7) << 7;
  return data;
}

void set_data (uint8_t data) {
  digitalWrite(dataB0, (data & (1 << 0)));
  digitalWrite(dataB1, (data & (1 << 1)));
  digitalWrite(dataB2, (data & (1 << 2)));
  digitalWrite(dataB3, (data & (1 << 3)));
  digitalWrite(dataB4, (data & (1 << 4)));
  digitalWrite(dataB5, (data & (1 << 5)));
  digitalWrite(dataB6, (data & (1 << 6)));
  digitalWrite(dataB7, (data & (1 << 7)));
}

uint8_t read_byte (uint32_t address) {
  set_address(address);
  return get_data();
}

void write_byte (uint32_t address, uint8_t data) {
  set_address(address);
  set_data(data);
  switch (chip) {
    case C16:
      digitalWrite(pinCE, HIGH);
      delay(15);
      digitalWrite(pinCE, LOW);
      break;
    case C32:
    case C64:
    case C128:
    case C256:
    case C512:
      digitalWrite(pinCE, LOW);
      delayMicroseconds(110);
      digitalWrite(pinCE, HIGH);
      break;
    case C1001:
    case C2001:
      digitalWrite(pinProgramC1001C2001, HIGH);
      delayMicroseconds(110);
      digitalWrite(pinProgramC1001C2001, LOW);
      break;
    case C4001:
    case C801:
    default:
      digitalWrite(pinCE, LOW);
      delayMicroseconds(110);
      digitalWrite(pinCE, HIGH);
      break;
  }
}

float get_voltage (void) {
  float vADC = (5. * analogRead(voltageControl)) / 1023.;
  float current = vADC / rBottom;
  return (current * (rTop + rBottom));
}
