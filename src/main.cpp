#include <Arduino.h>
#include <fmt/format.h>

#define SCK 4
#define DOUT 5
#define SCLK_LOW digitalWrite(SCK, 0)
#define SCLK_HIGH digitalWrite(SCK, 1)

enum REG_MODS {
  CH_SEL_A = 0b00,
  CH_SEL_B = 0b01,
  CH_SEL_CHIP_RETENTION = 0b01,
  CH_SEL_TEMPERATURE = 0b10,
  CH_SEL_INTERNAL_SHORT = 0b11,
  PGA_SEL_1 = 0b00 << 2,
  PGA_SEL_2 = 0b01 << 2,
  PGA_SEL_64 = 0b10 << 2,
  PGA_SEL_128 = 0b11 << 2,
  SPEED_SEL_10 = 0b00 << 4,
  SPEED_SEL_40 = 0b01 << 4,
  SPEED_SEL_640 = 0b10 << 4,
  SPEED_SEL_1280 = 0b11 << 4,
  REF_ON = 0b0 << 6,
  REF_OFF = 0b1 << 6
};

void CS1237Init()
{
	pinMode(SCK, OUTPUT);
	pinMode(DOUT, INPUT_PULLUP);
  delayMicroseconds(10);
	SCLK_LOW;
}

int CS1237isReady()
{
	return !digitalRead(DOUT);
}

void CS1237ClockUp()
{
	SCLK_HIGH;
	delayMicroseconds(10);
	SCLK_LOW;
	delayMicroseconds(10);
}

void CS1237WriteConfig()
{
	//no op

  //pinMode(DOUT, OUTPUT);

  while (!CS1237isReady());

  // 1.
  // Blank for 24 clock cycles, part of the spec
  for (int i = 0; i < 24; i++)
    CS1237ClockUp();

  // 2.
  // Skip the two register status reads, don't care yet
  CS1237ClockUp();
  CS1237ClockUp();

  // 3.
  // 27th SCLK pulls the /DRDY / DOUT high
  CS1237ClockUp();

  // 4.
  // Between clock 28 and 29 switch DOUT to an output
  CS1237ClockUp();
  pinMode(DOUT, OUTPUT);
  CS1237ClockUp();

  // 5.
  // Read 7 bits on input word to determine read or write
  char writeReg = 0x65 << 1;
  for (int i = 0; i < 7; i++)
  {
    digitalWrite(DOUT, 0x80 & writeReg);
    writeReg <<= 1;
    CS1237ClockUp();
    SCLK_LOW;
  }

  // 6.
  // Spare clock since we only write for now
  CS1237ClockUp();

  char settings = (CH_SEL_TEMPERATURE | PGA_SEL_128 | SPEED_SEL_1280 | REF_OFF);
  for (int i = 0; i < 8; i++)
  {
    digitalWrite(DOUT, 0x80 & settings);
    settings <<= 1;
    CS1237ClockUp();
  }

  digitalWrite(DOUT, 0);

	SCLK_LOW;
	CS1237ClockUp();
	CS1237ClockUp();
	CS1237ClockUp();
	SCLK_LOW;

  pinMode(DOUT, INPUT_PULLUP);
}

int CS1237ReadConfig()
{
  while (!CS1237isReady());

  // 1.
  // Blank for 24 clock cycles, part of the spec
  for (int i = 0; i < 24; i++)
    CS1237ClockUp();

  // 2.
  // Skip the two register status reads, don't care yet
  CS1237ClockUp();
  CS1237ClockUp();

  // 3.
  // 27th SCLK pulls the /DRDY / DOUT high
  CS1237ClockUp();

  // 4.
  // Between clock 28 and 29 switch DOUT to an output
  CS1237ClockUp();
  pinMode(DOUT, OUTPUT);
  CS1237ClockUp();

  // 5.
  // Read 7 bits on input word to determine read or write
  char writeReg = 0x56 << 1;
  for (int i = 0; i < 7; i++)
  {
    digitalWrite(DOUT, 0x80 & writeReg);
    writeReg <<= 1;
    CS1237ClockUp();
    SCLK_LOW;
  }

  // 6.
  //
  CS1237ClockUp();
  pinMode(DOUT, INPUT_PULLUP);

  char settings = 0;
  for (int i = 0; i < 8; i++)
  {
    CS1237ClockUp();
    settings = (settings << 1) | digitalRead(DOUT);
    SCLK_LOW;
  }

	SCLK_LOW;
	CS1237ClockUp();
	CS1237ClockUp();
	CS1237ClockUp();
	SCLK_LOW;

  return settings;
}

int CS1237Read()
{
	int tmp = 0;

	while (!CS1237isReady());

	for (int i = 0; i < 24; i++)
	{
    CS1237ClockUp();
		tmp = (tmp << 1) | digitalRead(DOUT);
		SCLK_LOW;
	}

	SCLK_LOW;
	CS1237ClockUp();
	CS1237ClockUp();
	CS1237ClockUp();
	SCLK_LOW;

	return tmp;
}

long getValue(int num)
{
	long weight = 0;
	for(int i = 0; i < num; i++)
	{
		weight += CS1237Read();;
	}
	return weight / num;
}

long offset = 0;

void tare()
{
  Serial.print("Interupt Triggered");
  offset = getValue(640);
}

void setup() {
  Serial.begin(115200);
  pinMode(24, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(24), tare, CHANGE);
  delayMicroseconds(10);
  CS1237Init();
  CS1237WriteConfig();
}

void loop() {
  if (!digitalRead(24))
    offset = getValue(128);

  Serial.print(fmt::format("Attempting: {:>10} {:>10}", CS1237Read() - offset, offset).c_str());
  Serial.println();
}
