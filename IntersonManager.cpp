//
// Created by Matthieu Heitz on 6/10/15.
// Copyright (c) 2015 Kitware Inc. All rights reserved.
//

#include <unistd.h>
#include "IntersonManager.h"

#include <time.h>
#include <fstream>

#ifndef ARRAYSIZE
#define ARRAYSIZE(A) (sizeof(A)/sizeof((A)[0]))
#endif

libusb_device_handle * m_gUSBHandle;
libusb_context* m_gUSBContex;
IntersonManager* m_gIntersonManager;

int gFrameNumber;


void LibUSBTransferCallback(struct libusb_transfer * transfer)
{
  std::cout<<"LibUSBTransferCallback is called"<<std::endl;
}

IntersonManager::IntersonManager()
{
  this->m_IsLibUSBConnected = false;
  IntersonManager("IntersonFMW_total.bix");
  gFrameNumber = 0;
}

IntersonManager::IntersonManager(const char* pathOfFirmware)
{
  this->m_PathOfFirmware = pathOfFirmware;
  this->ConnectIntersonUSProbe();
  m_gIntersonManager = this;
}


IntersonManager::~IntersonManager()
{
  if(this->m_IsLibUSBConnected)
    {
    this->CloseLibUSB();
    }
}

// This function initializes the Analog TGC (address 0x700 to 0x702)
// TGC must be of size 3, and contain the parameters of the desired TGC
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::initializeAnalogTGC(uInt8 TGC[3])
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0700;
  int r=1;
  uInt8 * data = new uInt8[wLength];

  // Send the 3 commands
  for (uInt16 i = 0; i < 3; i++)
    {
    data[0] = TGC[i];
    // If one of the transfer fails, r will be equal to 0 at the end
    r = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue+i, 0x00, data, wLength, VENDORCMD_TIMEOUT);
    if ( r != 1 )
      {
      std::cerr<<"Command initializeAnalogTGC failed"<<std::endl;
      return false;
      }
    }

  if (this->m_Verbose)
    {
    std::cout << "Command initializeAnalogTGC successfully sent" << std::endl;
    }
  return true;
}

// This function initializes the Digital TGC (address 0x500, 0x501, and 0x508 to 0x50F)
// TGC must be of size 10, and contain the parameters of the desired TGC
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::initializeDigitalTGC(uInt8 TGC[10])
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0700;
  int r=1;
  uInt8 * data = new uInt8[wLength];

  // Send the 3 commands
  for (uInt16 i = 0; i < 10; i++)
  {
    data[0] = TGC[i];
    // Trick for the addresses, because the aren't consecutive
    uInt16 offset = i<=1 ? i : i+6;
    // If one of the transfer fails, r will be equal to 0 at the end
    r = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue+offset, 0x00, data, wLength, VENDORCMD_TIMEOUT);
    if ( r != 1 )
    {
      std::cerr<<"Command initializeDigitalTGC failed"<<std::endl;
      return false;
    }
  }
  if (this->m_Verbose) std::cout << "Command initializeDigitalTGC successfully sent" << std::endl;
  return true;
}

// This function initializes the Motor Speed (address 0x801 to 0x807)
// speedArray must be of size 7, and contain the parameters of the desired speeds
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::initializeMotorSpeed(uInt8 speedArray[7])
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0801;
  int r=1;
  uInt8 * data = new uInt8[wLength];

  // Send the 7 commands
  for (uInt16 i = 0; i < 7; i++)
  {
    data[0] = speedArray[i];
    // If one of the transfer fails, r will be equal to 0 at the end
    r = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue+i, 0x00, data, wLength, VENDORCMD_TIMEOUT);
    //r = libusb_control_transfer(GetUSBHandle(), 0x40, 0xD8, wValue+i, 0x00, data, wLength, VENDORCMD_TIMEOUT);
    // Check if they are sent correctly
    if ( r != 1 )
    {
      std::cerr<<"Command initializeMotorSpeed failed"<<std::endl;
      return false;
    }
  }
  if(this->m_Verbose) std::cout<<"Command initializeMotorSpeed successfully sent"<<std::endl;
  return true;
}

// This function  initializes the Sampler Inc
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::initializeSamplerInc()
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0600;
  uInt8 * data = new uInt8[wLength];
  data[0] = 64;

  int r = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
  if ( r == wLength )
  {
    if(this->m_Verbose) std::cout<<"Command initializeSamplerInc successfully sent"<<std::endl;
    return true;
  }
  else
  {
    std::cerr<<"Command initializeSamplerInc failed"<<std::endl;
    return false;
  }
}


// This function returns the FPGA version, in the data array (size 1)
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::readFPGAVersion(uInt8 * data)
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0000;

  if(data == NULL)
  {
    data = new uInt8[wLength];
  }
  int r = libusb_control_transfer(m_gUSBHandle, 0xC0, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
  if ( r == wLength )
  {
    if (this->m_Verbose) std::cout << "Command readFPGAVersion successfully sent" << std::endl;
    return true;
  }
  else
  {
    std::cerr<<"Command readFPGAVersion failed"<<std::endl;
    return false;
  }
}

// This function reads the hard button state
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::readHardButton(uInt8 * data)
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0000;

  if(data == NULL)
  {
    std::cout<<"Allocating data"<<std::endl;
    data = new uInt8[wLength];
  }
  int r = libusb_control_transfer(m_gUSBHandle, 0xC0, 0xDA, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
  std::cout<<"Data = "<<*data<<std::endl;
  if ( r == wLength )
  {
    if(this->m_Verbose) std::cout<<"Command readHardButton successfully sent"<<std::endl;
    return true;
  }
  else
  {
    std::cerr<<"Command readHardButton failed"<<std::endl;
    return false;
  }
}

// This function reads "length" bytes in the OEM memory spaces, at "address"
// The read bytes are returned in "data"
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::readOEMSpace(int address, int length, uInt8 * data)
{
  if(data == NULL)
  {
    data = new uInt8[length];
  }
  int r = libusb_control_transfer(m_gUSBHandle, 0xC0, 0xD7, (uInt16)address, 0x00, data, (uInt16)length, VENDORCMD_TIMEOUT);
  if ( r == length )
  {
    if(this->m_Verbose)
    {
      std::cout<<"Command readOEMSpace successfully sent"<<std::endl;
      std::cout<<"Data received : "<<std::endl;
      for (int i = 0; i < r; i++)
      {
        std::cout<<std::hex<<(0xFF & (data[i]))<<" ";
        if((i+1) % 10 == 0) {
          std::cout<<std::endl;
          std::cout<<std::endl;
        }
      }
      std::cout<<std::endl;
    }
    return true;
  }
  else
  {
    std::cerr<<"Command readOEMSpace failed"<<std::endl;
    return false;
  }
}

// This function sets the High Voltage parameter on the probe
// The voltage must be an integer between 0 and 178
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::sendHighVoltage(int highVoltage)
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0201;
  uInt8 * data = new uInt8[wLength];
  if(highVoltage >= 0 && highVoltage <=178) {
    data[0] = (uInt8)highVoltage;
  }
  else
  {
    std::cerr<<"The voltage value is out of bound (should be: 0 <= voltage <= 178)"<<std::endl;
    return false;
  }

  int r = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
  if ( r == wLength )
  {
    if(this->m_Verbose) std::cout<<"Command sendHighVoltage successfully sent"<<std::endl;
    return true;
  }
  else
  {
    std::cerr<<"Command sendHighVoltage failed"<<std::endl;
    return false;
  }
}


// This function sets the Dynamic Range parameter on the probe (at address 0x900 and 0x901)
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::setDynamicRange()
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0900;
  uInt8 * data = new uInt8[wLength];
  data[0] = 39;

  int r1 = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);

  wValue = 0x0901;
  data[0] = 25;
  int r2 = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);

  if ( r1 == wLength && r2 == wLength)
  {
    if(this->m_Verbose) std::cout<<"Command sendDynamic successfully sent"<<std::endl;
    return true;
  }
  else
  {
    std::cerr<<"Command sendDynamic failed"<<std::endl;
    return false;
  }
}

// This function sets the frequency index, at the address 0x400
// The index must be an integer between 0 and 3
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::setFrequencyBandPassFilter(int frequencyIndex)
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0400;
  uInt8 * data = new uInt8[wLength];
  if(frequencyIndex >= 0 && frequencyIndex <=3) {
    data[0] = (uInt8)frequencyIndex;
  }
  else
  {
    std::cerr<<"The frequency index is out of bound (should be: 0 <= index <= 3)"<<std::endl;
    return false;
  }

  int r = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
  if ( r == wLength )
  {
    if(this->m_Verbose) std::cout<<"Command setFrequencyBandPassFilter successfully sent"<<std::endl;
    return true;
  }
  else
  {
    std::cerr<<"Command setFrequencyBandPassFilter failed"<<std::endl;
    return false;
  }
}

// This function sets the frequency index, at the address 0x100
// The index must be an integer between 0 and 31
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::setFrequencyIndex(int frequencyIndex)
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0100;
  uInt8 * data = new uInt8[wLength];
  if(frequencyIndex >= 0 && frequencyIndex <=31)
  {
    data[0] = (uInt8)frequencyIndex;
  }
  else
  {
    std::cerr<<"The frequency index is out of bound (should be: 0 <= index <= 31)"<<std::endl;
    return false;
  }

  int r = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);

  if ( r == wLength )
  {
    if(this->m_Verbose) std::cout<<"Command setFrequencyIndex successfully sent"<<std::endl;
    return true;
  }
  else
  {
    std::cerr<<"Command setFrequencyIndex failed"<<std::endl;
    return false;
  }

}

// This function sets the Frequency at the address 0x300 and 0x101
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::setFrequencyInit()
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0101;
  uInt8 * data = new uInt8[wLength];
  uInt8 data2;
  data[0] = 0;
  int r1 = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
  wValue = 0x0300;
  data[0] = 0;
  int r2 = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
  if ( r1 == wLength && r2 == wLength)
  {
    if(this->m_Verbose) std::cout<<"Command setFrequencyInit successfully sent"<<std::endl;
    return true;
  }
  else
  {
    std::cerr<<"Command setFrequencyInit failed"<<std::endl;
    return false;
  }
}

// This function enables (enable = true) or disables (enable = false) the High Voltage
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::setEnableHighVoltage(bool enable)
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0200;
  uInt8 * data = new uInt8[wLength];
  data[0] = (uInt8) (enable ? 1 : 0);

  int r = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
  if ( r == wLength )
  {
    if(this->m_Verbose) std::cout<<"Command setEnableHighVoltage successfully sent"<<std::endl;
    return true;
  }
  else
  {
    std::cerr<<"Command setEnableHighVoltage failed"<<std::endl;
    return false;
  }
}

// This function enables (enable = true) or disables (enable = false) the RF Decimator
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::setEnableRFDecimator(bool enable)
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0003;
  uInt8 * data = new uInt8[wLength];
  data[0] = (uInt8) (enable ? 1 : 0);

  int r = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
  if ( r == wLength )
  {
    if(this->m_Verbose) std::cout<<"Command setEnableRFDecimator successfully sent"<<std::endl;
    return true;
  }
  else
  {
    std::cerr<<"Command setEnableRFDecimator failed"<<std::endl;
    return false;
  }
}

//
bool IntersonManager::setMotorSpeed(float fps)
{
  if(fps - 15.0 < 1e-4)
  {
    uInt8 motorSpeed[] = {0x7A, 0x0D, 0x00, 0x00, 0x80, 0xA4, 0x32}; // 15.0 fps hex
    initializeMotorSpeed(motorSpeed);
  }
  else if(fps - 12.5 < 1e-4)
  {
  uInt8 motorSpeed[] = {0x93, 0x07, 0x00, 0x00, 0x93, 0xA4, 0x32}; // 12.5 fps hex
    initializeMotorSpeed(motorSpeed);
  }
  else if(fps - 10.0 < 1e-4)
  {
  uInt8 motorSpeed[] = {0xB9, 0x04, 0x00, 0x00, 0xB7, 0xA4, 0x32}; // 10.0 fps hex
    initializeMotorSpeed(motorSpeed);
  }
  else
  {
    std::cerr<<"Motor speed not supported.\nSupported values are: 10.0, 12.5, 15.0"<<std::endl;
    return false;
  }
  return true;
}

// This function starts (start = true) or stops (start = false) the probe motor
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::setStartMotor(bool start)
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0800;
  uInt8 * data = new uInt8[wLength];
  data[0] = (uInt8) (start ? 1 : 0);

  int r = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
  if ( r == wLength )
  {
    if(this->m_Verbose) std::cout<<"Command setStartMotor successfully sent"<<std::endl;
    return true;
  }
  else
  {
    std::cerr<<"Command setStartMotor failed"<<std::endl;
    return false;
  }
}

// This function starts the BMode Acquisition
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::startBMode()
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0000;
  uInt8 * data = new uInt8[wLength];
  data[0] = 1;

  int r = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
  if ( r == wLength )
  {
    if(this->m_Verbose) std::cout<<"Command startBMode successfully sent"<<std::endl;
    return true;
  }
  else
  {
    std::cerr<<"Command startBMode failed"<<std::endl;
    return false;
  }
}

// This function starts the RFMode Acquisition
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::startRFMode()
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0000;
  uInt8 * data = new uInt8[wLength];
  data[0] = 2;

  int r = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
  if ( r == wLength )
  {
    if(this->m_Verbose) std::cout<<"Command startRFMode successfully sent"<<std::endl;
    return true;
  }
  else
  {
    std::cerr<<"Command startRFMode failed"<<std::endl;
    return false;
  }
}

// This function stops any acquisition started
// Do not call this function without following the End of Acquisition Sequence
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::stopAcquisition()
{
  // Data to send
  uInt16 wLength = 1;
  uInt16 wValue = 0x0000;
  uInt8 * data = new uInt8[wLength];
  data[0] = 0;

  int r = libusb_control_transfer(m_gUSBHandle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
  if ( r == wLength )
  {
    if(this->m_Verbose) std::cout<<"Command stopAcquisition successfully sent"<<std::endl;
    return true;
  }
  else
  {
    std::cerr<<"Command stopAcquisition failed"<<std::endl;
    return false;
  }
}

// TODO
// This function writes the "length" first bytes of "data" in the OEM memory space at "address"
// Returns true if the command is sent, and false if it failed.
bool IntersonManager::writeOEMSpace(int address, int length, uInt8 * data)
{
  if(data == NULL)
  {
    std::cerr<<"array 'data' is NULL, there is nothing to send"<<std::endl;
    return false;
  }

  int r = libusb_control_transfer(m_gUSBHandle, 0xC0, 0xD7, (uInt16)address, 0x00, data, (uInt16)length, VENDORCMD_TIMEOUT);
  if ( r == length ) {
    if(this->m_Verbose) std::cout<<"Command writeOEMSpace successfully sent"<<std::endl;
    return true;
  } else {
    std::cerr<<"Command writeOEMSpace failed"<<std::endl;
    return false;
  }

}

libusb_device_handle * IntersonManager::getHandle()
{
  return m_gUSBHandle;
}

void IntersonManager::Error(int err)
{
  if (err == 0)
  {
    std::cout << "libusb: Operation successful" << std::endl;
  }
  else
  {
    std::cerr<<"libusb error: ";
    if (err == -1)
    {
      std::cerr << "Input/output error" << std::endl;
    }
    else if (err == -2)
    {
      std::cerr << "Invalid parameter" << std::endl;
    }
    else if (err == -3)
    {
      std::cerr << "Access denied (insufficient permissions)" << std::endl;
    }
    else if (err == -4)
    {
      std::cerr << "No such device. Disconnected...?" << std::endl;
    }
    else if (err == -5)
    {
      std::cerr << "Entity not found" << std::endl;
    }
    else if (err == -6)
    {
      std::cerr << "Resource busy" << std::endl;
    }
    else if (err == -7)
    {
      std::cerr << "Operation timed out" << std::endl;
    }
    else if (err == -8)
    {
      std::cerr << "Overflow" << std::endl;
    }
    else if (err == -9)
    {
      std::cerr << "Pipe error" << std::endl;
    }
    else if (err == -10)
    {
      std::cerr << "System call interrupted, ( due to signal ? )" << std::endl;
    }
    else if (err == -11)
    {
      std::cerr << "Insufficient memory" << std::endl;
    }
    else if (err == 12)
    {
      std::cerr << "Operation not supported/implemented" << std::endl;
    }
    else
    {
      std::cerr << "Unknown internal error" << std::endl;
    }
  }

//    libusb_error {
//            LIBUSB_SUCCESS = 0, LIBUSB_ERROR_IO = -1, LIBUSB_ERROR_INVALID_PARAM = -2, LIBUSB_ERROR_ACCESS = -3,
//            LIBUSB_ERROR_NO_DEVICE = -4, LIBUSB_ERROR_NOT_FOUND = -5, LIBUSB_ERROR_BUSY = -6, LIBUSB_ERROR_TIMEOUT = -7,
//            LIBUSB_ERROR_OVERFLOW = -8, LIBUSB_ERROR_PIPE = -9, LIBUSB_ERROR_INTERRUPTED = -10, LIBUSB_ERROR_NO_MEM = -11,
//            LIBUSB_ERROR_NOT_SUPPORTED = -12, LIBUSB_ERROR_OTHER = -99
//    }
}


// TODO
bool IntersonManager::initializeProbe()
{
  // Set Frequency addr = 0x101 to '0' and addr = 0x300 to '0'
  setFrequencyInit();

  // Initialize Analog TGC
  uInt8 * analogTGC = new uInt8[3];
  initializeAnalogTGC(analogTGC);

  // Initialize Digital TGC
  uInt8 * digitalTGC = new uInt8[10];
  initializeDigitalTGC(digitalTGC);

  // Send Dynamic
  setDynamicRange();

  // Read FPGA Version
  uInt8 FPGAversion;
  readFPGAVersion(&FPGAversion);
  std::cout<<"FPGA Version = "<<std::hex<<+FPGAversion<<std::endl;


  // Initialize Motor Speed

//  uInt8 motorSpeed[] = {0xB9, 0x04, 0x00, 0x00, 0xB7, 0xA4, 0x32}; // 10.0 fps hex
//  uInt8 motorSpeed[] = {0x93, 0x07, 0x00, 0x00, 0x93, 0xA4, 0x32}; // 12.5 fps hex
  uInt8 motorSpeed[] = {0x7A, 0x0D, 0x00, 0x00, 0x80, 0xA4, 0x32}; // 15.0 fps hex
  initializeMotorSpeed(motorSpeed);


  // Set Frequency addr = 0x100 (Frequency)

//  setFrequencyIndex(0); // 20 MHz
//  setFrequencyIndex(1); // 15 MHz
//  setFrequencyIndex(11); // 4.29 MHz
  setFrequencyIndex(14); // 3.53 MHz
//  setFrequencyIndex(15); // 3.33 MHz
//  setFrequencyIndex(19); // 2.73 MHz
//  setFrequencyIndex(28); // 1.94 MHz


  // Set Frequency addr = 0x400 (band pass Filter)

//  setFrequencyBandPassFilter(0); // 2.87MHz
//  setFrequencyBandPassFilter(1); // 3.37MHz
  setFrequencyBandPassFilter(2); // 3.87MHz
//  setFrequencyBandPassFilter(3); // 4.37MHz


  // Send High Voltage
  // API doc says param is a %
  // Voltage should be between 0 and 178

//  sendHighVoltage(0);
//  sendHighVoltage(50);
  sendHighVoltage(100);
//  sendHighVoltage(150);
//  sendHighVoltage(178);
  this->m_IsLibUSBConnected = true;
  return true;
}

bool IntersonManager::initializeProbeRFMode()
{

  // Set Frequency addr = 0x101 to '0' and addr = 0x300 to '0'
  if(!setFrequencyInit())
    {
    std::cerr << "Failed setFrequencyInit()" << std::endl;
    }

  // Initialize Analog TGC
  uInt8 * analogTGC = new uInt8[3];
  if(!initializeAnalogTGC(analogTGC))
    {
    std::cerr << "Failed initializeAnalogTGC()" << std::endl;
    }

  // Initialize Digital TGC
  uInt8 * digitalTGC = new uInt8[10];
  if(!initializeDigitalTGC(digitalTGC))
    {
    std::cerr << "Failed initializeDigitalTGC()" << std::endl;
    }

  // Send Dynamic
  if(!setDynamicRange())
    {
    std::cerr << "Failed setDynamicRange()" << std::endl;
    }

  // Read FPGA Version
  uInt8 FPGAversion;
  if(!readFPGAVersion(&FPGAversion))
    {
    std::cerr << "Failed readFPGAVersion()" << std::endl;
    }
  else
    {
    std::cout<<"FPGA Version = "<<std::hex<<+FPGAversion<<std::endl;
    }


  // Initialize Motor Speed

////  uInt8 motorSpeed[] = {0xB9, 0x04, 0x00, 0x00, 0xB7, 0xA4, 0x32}; // 10.0 fps hex
////  uInt8 motorSpeed[] = {0x93, 0x07, 0x00, 0x00, 0x93, 0xA4, 0x32}; // 12.5 fps hex
    uInt8 motorSpeed[] = {0x7A, 0x0D, 0x00, 0x00, 0x80, 0xA4, 0x32}; // 15.0 fps hex
    initializeMotorSpeed(motorSpeed);
//  if(!this->setMotorSpeed(15.0f))
//    {
//    std::cerr << "Failed setMotorSpeed()" << std::endl;
//    }


  // Set Frequency addr = 0x100 (Frequency)

  if(!setFrequencyIndex(2))
    {
    std::cerr << "Failed setFrequencyIndex()" << std::endl;
    }
     // 20 MHz
//  setFrequencyIndex(1); // 15 MHz
//  setFrequencyIndex(11); // 4.29 MHz
  //setFrequencyIndex(14); // 3.53 MHz
//  setFrequencyIndex(15); // 3.33 MHz
//  setFrequencyIndex(19); // 2.73 MHz
//  setFrequencyIndex(28); // 1.94 MHz


  // Set Frequency addr = 0x400 (band pass Filter)
  //  setFrequencyBandPassFilter(0); // 2.87MHz
  //  setFrequencyBandPassFilter(1); // 3.37MHz
  //  setFrequencyBandPassFilter(2); // 3.87MHz
  //  setFrequencyBandPassFilter(3); // 4.37MHz
  if(!setFrequencyBandPassFilter(2))
    {
    std::cerr << "Failed setFrequencyBandPassFilter()" << std::endl;
    }

  this->setEnableHighVoltage(true);
  this->initializeSamplerInc();
  // Send High Voltage
  // API doc says param is a %
  // Voltage should be between 0 and 178
  if(!sendHighVoltage(100))
    {
    std::cerr << "Failed setFrequencyInit()" << std::endl;
    }

  this->m_IsLibUSBConnected = true;
  return true;
}

// Set verbose, and returns the value
void IntersonManager::setVerbose(bool verbose)
{
  this->m_Verbose = verbose;

  if(this->m_Verbose)
    {
      libusb_set_debug(m_gUSBContex, LIBUSB_LOG_LEVEL_DEBUG);
    }
  else
    {
      libusb_set_debug(m_gUSBContex, LIBUSB_LOG_LEVEL_NONE);
    }
}

// TODO
// This function starts the acquisition, using the Sequence of Initialization and Acquisition
// B-mode: acquisitionMode = 0
// RF-mode: acquisitionMode = 1
bool IntersonManager::startAcquisitionRoutine(int acquisitionMode)
{
  if(acquisitionMode == 0 || acquisitionMode == 1) {
    m_AcquisitionMode = acquisitionMode;
  } else {
    std::cerr<<"Bad parameter value : acquisitionMode not recognized"<<std::endl;
    std::cerr<<"Possible parameters are : 0 for B-Mode, 1 for RF-Mode"<<std::endl;
    return false;
  }

  // Calculate the Scan Converter
  // TODO : Calculate the Scan Converter

  // Can readjust Motor Speed, Freq 0x100 and 0x400, and HighVoltage

  // Enable High Voltage
  setEnableHighVoltage(true);

  // Enable RF decimator ( for probe 3.5MHz and RF acquisition)
  if(m_AcquisitionMode == 1)
  {
    //setEnableRFDecimator(true);
  }

  // Initialize Sampler Inc
  initializeSamplerInc();

  // Start the Motor
  setStartMotor(true);

  usleep(1e5);
  // Start Acquisition ( Start B Mode or Start RF mode)
  if(m_AcquisitionMode == 0) {
    startBMode();
  }
  if(m_AcquisitionMode == 1) {
    startRFMode();
  }


  return true;

}

// TODO
// This function stops the acquisition, using the Sequence of Initialization and Acquisition
bool IntersonManager::stopAcquisitionRoutine()
{
  //setVerbose(true);

//  // Stop every active acquisition threads
//  if(exec != null) {
//    exec.shutdown();
//  }

  // Stop the Acquisition
  stopAcquisition();

  usleep(1e5);

  // Stop the Motor
  setStartMotor(false);

  // Disable High Voltage
  setEnableHighVoltage(false);

  // Disable RF decimator ( if done with RF acquisition)
  if(m_AcquisitionMode == 1) setEnableRFDecimator(false);

  return true;
}

//int IntersonManager::AcquireUSspectroscopyFrames(std::vector<int>& frequency, std::vector<int>& power)
int IntersonManager::AcquireUSspectroscopyFrames(std::list<int>& frequency, std::list<int>& power)
{

  this->initializeProbeRFMode();
  // Setting the Motor speed as 15 fps
  this->setMotorSpeed(15.0f);

  // Enable High Voltage
  this->setEnableHighVoltage(true);

  // 15 M samples/sec
  this->setEnableRFDecimator(false);

  // Initialize Sampler Inc
  this->initializeSamplerInc();


  // Run Motor
  this->setStartMotor(true);
  //sleep(1);
  usleep(2e5);

  this->collector.Start("07.Data_Set");
  for(int findex = 0; findex < 3; ++findex)
    {
    for(int pindex = 0; pindex < 1; ++pindex)
      {
      gFrameNumber++;
      AcquireUSspectroscopyFrame(1, 100);
      }
    }
  this->collector.Stop("07.Data_Set");

  // Stop Motor
  this->setStartMotor(false);

  // Disable High Voltage
  this->setEnableHighVoltage(false);

  this->setEnableRFDecimator(false);

  this->stopAcquisitionRoutine();

  this->collector.Report();
  return 1;
}

int IntersonManager::AcquireUSspectroscopyFrame(int frequency, int power)
{

  //--- To de deleted
  int nRows = 241;
  int nColumn = 2048;
  int FrameSize  = nRows * nColumn;
  short* buffer = new short[FrameSize];
  int DataSize = FrameSize*sizeof(short);
  // ---

  // 1. Set Frequency
  this->setFrequencyIndex(frequency);
  // 2. Set Power
  this->sendHighVoltage(power);
  // 3  Run Data
  this->startRFMode();
  // 4. Waiting for a new Frame (blocking mode)
  int r = GetNewFrame((unsigned char*)buffer, DataSize);
  // 5. Stop Data
  this->stopAcquisition();


  //--- To de deleted
  std::string filepath = std::to_string(gFrameNumber)+std::string(".raw");
  std::ofstream outfile(filepath.c_str(), std::ofstream::binary);
  outfile.write((char*)buffer, DataSize);
  outfile.close();
  delete buffer;
  buffer = NULL;
  // ---

  return 1;
}


int IntersonManager::GetNewFrame(unsigned char* buffer, int DataSize)
{
  int           transferred = 1;
  int r = libusb_bulk_transfer(m_gUSBHandle, endpoint, buffer, DataSize, &transferred, VENDORCMD_TIMEOUT);

  return r;
}

void IntersonManager::CloseLibUSB()
{
  std::cout << "ClodeLibUSB-0" << std::endl;
  ReleaseUSBInterface();
  std::cout << "ClodeLibUSB-1" << std::endl;
  ExitLibUSB();
  std::cout << "ClodeLibUSB-2" << std::endl;
  this->m_IsLibUSBConnected = false;
}

// protected

bool IntersonManager::ConnectIntersonUSProbe()
{
  if(!TryConnectIntersonUSProbe())
    {
    // Search an attached Interson USBProbe
    if(!FindIntersonUSProbe())
      {
      std::cerr<<"No Interson USB Probe is attached."<<std::endl;
      return false;
      }

    // Upload the firmware to an attached USProbe
    if(!UploadFirmwareToIntersonUSProbe(this->m_PathOfFirmware.c_str()))
      {
      std::cerr<<"Failed Uploaded the firmware"<<std::endl;
      return false;
      }

    // Retry to connect an attached Interson USB Probe
    if(!TryConnectIntersonUSProbe())
      {
      std::cerr<<"Cannot connected the attached Interson USB Probe."<<std::endl;
      return false;
      }
    }

  if(libusb_kernel_driver_active(m_gUSBHandle, 0) == 1)  //find out if kernel driver is attached
  {
    std::cout<<"Kernel Driver Active"<<std::endl;
    if(libusb_detach_kernel_driver(m_gUSBHandle, 0) == 0) //detach it
      {
      std::cout<<"Kernel Driver Detached!"<<std::endl;
      }
    }

  libusb_set_configuration(m_gUSBHandle, 1);
  libusb_claim_interface(m_gUSBHandle, 0);

  if(this->m_Verbose)
  {
    std::cerr << "Interson US Probe 3.5 is connected!" << std::endl;
  }

  return true;
}

bool IntersonManager::TryConnectIntersonUSProbe()
{
  InitializeLibUSB();
  m_gUSBHandle = libusb_open_device_with_vid_pid(m_gUSBContex, 0x1921, 0x0090);

 if(m_gUSBHandle == NULL)
    {
    std::cerr<<"Impossible to open device 1921:0090"<<std::endl;
    ExitLibUSB();
    return this->m_IsLibUSBConnected = false;
    }

  return true;

}

bool IntersonManager::FindIntersonUSProbe()
{
  InitializeLibUSB();

  libusb_device **devs;

  if (libusb_get_device_list(NULL, &devs) < 0)
    {
    std::cerr << "No USB Device" << std::endl;
    this->ExitLibUSB();
    return this->m_IsLibUSBConnected = false;
    }

  m_gUSBHandle = libusb_open_device_with_vid_pid(m_gUSBContex, 0x1921, 0x9090);

  if (m_gUSBHandle == NULL)
    {
    std::cerr << "libusb_open() failed\n" <<std::endl;
    this->ExitLibUSB();
    return this->m_IsLibUSBConnected = false;
    }

  /* We need to claim the first interface */
  libusb_set_auto_detach_kernel_driver(m_gUSBHandle, 1);
  this->m_Status = libusb_claim_interface(m_gUSBHandle, 0);

  if (this->m_Status != LIBUSB_SUCCESS)
    {
    std::cerr << "libusb_claim_interface failed: %s\n" << libusb_error_name(this->m_Status) << std::endl;
    this->ExitLibUSB();
    return this->m_IsLibUSBConnected = false;
    }

  return true;
}

bool IntersonManager::UploadFirmwareToIntersonUSProbe(const char* path)
{
  this->m_Status = ezusb_load_ram(m_gUSBHandle, this->m_PathOfFirmware.c_str(),
                                  3, IMG_TYPE_BIX, 0);

  if (this->m_Status < 0)
    {
      std::cerr<<"ezusb_load_ram() failed: "<< libusb_error_name(this->m_Status) << std::endl;
      this->ExitLibUSB();
      return this->m_IsLibUSBConnected = false;
    }

  this->CloseLibUSB();
  this->m_IsLibUSBConnected = false;
  usleep(3e6);
  return true;
}

//const bool IntersonManager::InitializeLibUSB()
bool IntersonManager::InitializeLibUSB()
{
  int status = libusb_init(&m_gUSBContex);
  if (status < 0)
    {
      std::cerr<<"libusb_init() failed: "<< libusb_error_name(status) << std::endl;
      this->ExitLibUSB();
      return false;
    }

  return true;
}

//const void IntersonManager::ExitLibUSB()
void IntersonManager::ExitLibUSB()
{
  libusb_exit(m_gUSBContex);
}

//const void IntersonManager::ReleaseUSBInterface()
void IntersonManager::ReleaseUSBInterface()
{
  libusb_release_interface(m_gUSBHandle, 0);
  libusb_close(m_gUSBHandle);
}

void IntersonManager::InitializeLookupTables()
{
  this->m_LookUpFrequencyIndex.clear();
  this->m_LookUpFrequencyIndex[20.0]        =  0; // 20.0 MHz
  this->m_LookUpFrequencyIndex[15.0]        =  1; // 15.0 MHz
  this->m_LookUpFrequencyIndex[12.0]        =  2; // 12.0 MHz
  this->m_LookUpFrequencyIndex[10.0]        =  3; // 10.0 MHz
  this->m_LookUpFrequencyIndex[8.57]        =  4; // 8.57 MHz
  this->m_LookUpFrequencyIndex[7.50]        =  5; // 7.50 MHz
  this->m_LookUpFrequencyIndex[6.67]        =  6; // 6.67 MHz
  this->m_LookUpFrequencyIndex[6.00]        =  7; // 6.00 MHz
  this->m_LookUpFrequencyIndex[5.45]        =  8; // 5.45 MHz
  this->m_LookUpFrequencyIndex[5.00]        =  9; // 5.00 MHz
  this->m_LookUpFrequencyIndex[4.62]        = 10; // 4.62 MHz
  this->m_LookUpFrequencyIndex[4.29]        = 11; // 4.29 MHz
  this->m_LookUpFrequencyIndex[4.00]        = 12; // 4.00 MHz
  this->m_LookUpFrequencyIndex[3.75]        = 13; // 3.75 MHz
  this->m_LookUpFrequencyIndex[3.53]        = 14; // 3.53 MHz
  this->m_LookUpFrequencyIndex[3.33]        = 15; // 3.33 MHz
  this->m_LookUpFrequencyIndex[3.16]        = 16; // 3.16 MHz
  this->m_LookUpFrequencyIndex[3.00]        = 17; // 3.00 MHz
  this->m_LookUpFrequencyIndex[2.86]        = 18; // 2.86 MHz
  this->m_LookUpFrequencyIndex[2.73]        = 19; // 2.73 MHz
  this->m_LookUpFrequencyIndex[2.61]        = 20; // 2.61 MHz
  this->m_LookUpFrequencyIndex[2.50]        = 21; // 2.50 MHz
  this->m_LookUpFrequencyIndex[2.40]        = 22; // 2.40 MHz
  this->m_LookUpFrequencyIndex[2.31]        = 23; // 2.31 MHz
  this->m_LookUpFrequencyIndex[2.22]        = 24; // 2.22 MHz
  this->m_LookUpFrequencyIndex[2.14]        = 25; // 2.14 MHz
  this->m_LookUpFrequencyIndex[2.07]        = 26; // 2.07 MHz
  this->m_LookUpFrequencyIndex[2.00]        = 27; // 2.00 MHz
  this->m_LookUpFrequencyIndex[1.94]        = 28; // 1.94 MHz
  this->m_LookUpFrequencyIndex[1.88]        = 29; // 1.88 MHz
  this->m_LookUpFrequencyIndex[1.82]        = 30; // 1.82 MHz
  this->m_LookUpFrequencyIndex[1.76]        = 31; // 1.76 MHz

  this->m_LookUpBandPassFilerIndexGP35.clear();
  this->m_LookUpBandPassFilerIndexGP35[2.87] = 0;
  this->m_LookUpBandPassFilerIndexGP35[3.37] = 1;
  this->m_LookUpBandPassFilerIndexGP35[3.87] = 2;
  this->m_LookUpBandPassFilerIndexGP35[4.37] = 3;

  this->m_LookUpBandPassFilerIndexGP75.clear();
  this->m_LookUpBandPassFilerIndexGP75[5.75] = 0;
  this->m_LookUpBandPassFilerIndexGP75[6.75] = 1;
  this->m_LookUpBandPassFilerIndexGP75[7.75] = 2;
  this->m_LookUpBandPassFilerIndexGP75[8.75] = 3;

}

