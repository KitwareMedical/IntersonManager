//
// Created by Matthieu Heitz on 6/10/15.
// Copyright (c) 2015 Kitware Inc. All rights reserved.
//

#include <unistd.h>
#include "IntersonManager.h"


IntersonManager::IntersonManager(libusb_device_handle * handle)
{
  if(handle != NULL)
  {
    this->m_Handle = handle;
  }
  else
  {
    std::cerr<<"Invalid handle passed"<<std::endl;
  }
  setVerbose(false);
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
  for (uInt16 i = 0; i < 3; i++) {
    data[0] = TGC[i];
    // If one of the transfer fails, r will be equal to 0 at the end
    r = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue+i, 0x00, data, wLength, VENDORCMD_TIMEOUT);
    if ( r != 1 )
    {
      std::cerr<<"Command initializeAnalogTGC failed"<<std::endl;
      return false;
    }
  }
  if (this->m_Verbose) std::cout << "Command initializeAnalogTGC successfully sent" << std::endl;
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
    r = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue+offset, 0x00, data, wLength, VENDORCMD_TIMEOUT);
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
    r = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue+i, 0x00, data, wLength, VENDORCMD_TIMEOUT);
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

  int r = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
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
  int r = libusb_control_transfer(this->m_Handle, 0xC0, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
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
  int r = libusb_control_transfer(this->m_Handle, 0xC0, 0xDA, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
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
  int r = libusb_control_transfer(this->m_Handle, 0xC0, 0xD7, (uInt16)address, 0x00, data, (uInt16)length, VENDORCMD_TIMEOUT);
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

  int r = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
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

  int r1 = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);

  wValue = 0x0901;
  data[0] = 25;
  int r2 = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);

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

  int r = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
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

  int r = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
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
  data[0] = 0;

  int r1 = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);

  wValue = 0x0300;
  data[0] = 0;
  int r2 = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);

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

  int r = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
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

  int r = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
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
    std::cout<<"Motor speed not supported.\nSupported values are: 10.0, 12.5, 15.0"<<std::endl;
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

  int r = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
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

  int r = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
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

  int r = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
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

  int r = libusb_control_transfer(this->m_Handle, 0x40, 0xD8, wValue, 0x00, data, wLength, VENDORCMD_TIMEOUT);
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

  int r = libusb_control_transfer(this->m_Handle, 0xC0, 0xD7, (uInt16)address, 0x00, data, (uInt16)length, VENDORCMD_TIMEOUT);
  if ( r == length ) {
    if(this->m_Verbose) std::cout<<"Command writeOEMSpace successfully sent"<<std::endl;
    return true;
  } else {
    std::cerr<<"Command writeOEMSpace failed"<<std::endl;
    return false;
  }

}



libusb_device_handle * IntersonManager::getHandle() const
{
  return m_Handle;
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



  return true;
}

// Set verbose, and returns the value
void IntersonManager::setVerbose(bool verbose)
{
  this->m_Verbose = verbose;
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
    setEnableRFDecimator(true);
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
  setVerbose(true);

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


