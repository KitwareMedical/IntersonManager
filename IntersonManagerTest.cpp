//
// Created by Matthieu Heitz on 6/16/15.
// Copyright (c) 2015 Kitware Inc. All rights reserved.
//

#include "IntersonManagerTest.h"


#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "libusb.h"
#include "IntersonManager.h"


IntersonManagerTest::IntersonManagerTest(IntersonManager * manager)
{
  if(manager != NULL)
  {
    this->m_IntMng = manager;
  }
  else
  {
    std::cerr<<"Null pointer passed. Initialize the IntersonManager before passing it."<<std::endl;
  }
  this->m_IntMng = manager;
  this->m_IntMng->setVerbose(true);
  this->m_IntMng->initializeProbe();
}

IntersonManagerTest::~IntersonManagerTest()
{

}


void IntersonManagerTest::printDevices(libusb_device ** deviceList)
{
  libusb_device *dev;
  int i = 0, j = 0;
  uint8_t path[8];
  uint8_t string_index[3];	// indexes of the string descriptors
  char string[128];

  while ((dev = deviceList[i]) != NULL)
  {
    printf("\n");
    struct libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(dev, &desc);
    if (r < 0)
    {
      fprintf(stderr, "failed to get device descriptor");
      return;
    }

    string_index[0] = desc.iManufacturer;
    string_index[1] = desc.iProduct;
    string_index[2] = desc.iSerialNumber;
    printf("%04x:%04x (bus %d, device %d)",
           desc.idVendor, desc.idProduct,
           libusb_get_bus_number(dev), libusb_get_device_address(dev));

    r = libusb_get_port_numbers(dev, path, sizeof(path));
    if (r > 0) {
      printf(" path: %d", path[0]);
      for (j = 1; j < r; j++)
        printf(".%d", path[j]);
    }
    printf("\n");

    libusb_device_handle *handle;
    r = libusb_open(dev, &handle);

    if(r == LIBUSB_ERROR_ACCESS)
    {
      std::cout<<" Insufficient permission"<<std::endl;
    }

    if (r == 0 && handle != NULL)
    {
      for (int k = 0; k < 3; k++)
      {
        if (string_index[k] == 0)
        {
          continue;
        }
        if (libusb_get_string_descriptor_ascii(handle, string_index[k], (unsigned char *) string, 128) >= 0)
        {
          printf("String Desc(0x%02X): \"%s\"\n", string_index[k], string);
        }
      }
      libusb_close(handle);
    }
    i++;
  }
}


int IntersonManagerTest::TestHardButton()
{
  // Testing HardButton - using usleep()
  for (int i = 0; i < 30; ++i)
  {
    uInt8 state = 0xAC;
    this->m_IntMng->readHardButton(&state);
    std::cout<<"HardButton state = "<<+state<<std::endl;
    usleep(300000);
  }
  return 0;
}

int IntersonManagerTest::TestMotor1Speed()
{
  // Testing Motor - 1 speed
  uInt8 motorSpeed15[] = {0x7A, 0x0D, 0x00, 0x00, 0x80, 0xA4, 0x32}; // 15.0 fps
  this->m_IntMng->initializeMotorSpeed(motorSpeed15);
  this->m_IntMng->setStartMotor(true);
  sleep(1);
  this->m_IntMng->setStartMotor(false);

  return 0;
}

int IntersonManagerTest::TestMotor3Speeds()
{
  // Testing Motor - 3 speeds
  uInt8 motorSpeed10[] = {0xB9, 0x04, 0x00, 0x00, 0xB7, 0xA4, 0x32}; // 10.0 fps
  this->m_IntMng->initializeMotorSpeed(motorSpeed10);
  this->m_IntMng->setStartMotor(true);
  sleep(1);
  this->m_IntMng->setStartMotor(false);
  uInt8 motorSpeed12_5[] = {0x93, 0x07, 0x00, 0x00, 0x93, 0xA4, 0x32}; // 12.5 fps
  this->m_IntMng->initializeMotorSpeed(motorSpeed12_5);
  this->m_IntMng->setStartMotor(true);
  sleep(1);
  this->m_IntMng->setStartMotor(false);
  uInt8 motorSpeed15[] = {0x7A, 0x0D, 0x00, 0x00, 0x80, 0xA4, 0x32}; // 15.0 fps
  this->m_IntMng->initializeMotorSpeed(motorSpeed15);
  this->m_IntMng->setStartMotor(true);
  sleep(1);
  this->m_IntMng->setStartMotor(false);

  return 0;
}

int IntersonManagerTest::TestAcquisition()
{
  // Testing Acquisition - B-Mode
  this->m_IntMng->startAcquisitionRoutine(0);
  sleep(1);
  this->m_IntMng->stopAcquisitionRoutine();

  return 0;
}


int IntersonManagerTest::TestSyncBulk_FrameByFrame_Overview()
{
  // Testing Receiving Data : B-Mode, Synchronous Bulk
  uInt8 motorSpeed10[] = {0xB9, 0x04, 0x00, 0x00, 0xB7, 0xA4, 0x32}; // 10.0 fps
  uInt8 motorSpeed15[] = {0x7A, 0x0D, 0x00, 0x00, 0x80, 0xA4, 0x32}; // 15.0 fps
  this->m_IntMng->initializeMotorSpeed(motorSpeed10);
  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_NONE);
  std::cout<<std::dec;


  this->m_IntMng->startAcquisitionRoutine(0);

  int r;
  int nRows = 241;
  int nColumn = 1024;
  for (int i = 0; i < 20; ++i) // 20 frames
  {
    int transferSize = nRows*nColumn;
    unsigned char endpoint = 0x82;
    unsigned char * buffer = new unsigned char[transferSize];
    int transferred = 1;
    unsigned int BULK_TRANSFER_TIMEOUT = 1000;

    libusb_device_handle * handle = this->m_IntMng->getHandle();
    r = libusb_bulk_transfer(handle, endpoint, buffer, transferSize, &transferred, BULK_TRANSFER_TIMEOUT);

    // Terminal max size
    int maxTermRow = 25;
    int maxTermColumn = 25;

    // Overview of the frame
    for (int k = 0; k < nRows * nColumn; k += nColumn*(nRows/maxTermRow))
    {
      for (int j = 0; j < nColumn; j+= nColumn/maxTermColumn)
      {
        std::cout << +buffer[k+j] << " ";
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
  this->m_IntMng->stopAcquisitionRoutine();


  return 0;
}


int IntersonManagerTest::TestSyncBulk_FrameByFrame_CheckFrameNumbers()
{
  // Testing Receiving Data : B-Mode, Synchronous Bulk
  uInt8 motorSpeed10[] = {0xB9, 0x04, 0x00, 0x00, 0xB7, 0xA4, 0x32}; // 10.0 fps
  uInt8 motorSpeed15[] = {0x7A, 0x0D, 0x00, 0x00, 0x80, 0xA4, 0x32}; // 15.0 fps
  this->m_IntMng->initializeMotorSpeed(motorSpeed15);
  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_NONE);
  std::cout<<std::dec;

  this->m_IntMng->startAcquisitionRoutine(0);

  int r;
  for (int i = 0; i < 30; ++i) // 30 frames
  {
    int transferSize = 241 * 1024;
    unsigned char endpoint = 0x82;
    unsigned char *buffer = new unsigned char[transferSize];
    int transferred = 1;
    unsigned int BULK_TRANSFER_TIMEOUT = 1000;

    libusb_device_handle *handle = this->m_IntMng->getHandle();
    r = libusb_bulk_transfer(handle, endpoint, buffer, transferSize, &transferred, BULK_TRANSFER_TIMEOUT);

    std::cout << "Frame numbers:" << std::hex << std::endl;
    for (int k = 0; k < 241 * 1024; k += 1024)
    {
      std::cout << +buffer[k] << " ";
    }
    std::cout << std::dec << std::endl;
  }

  this->m_IntMng->stopAcquisitionRoutine();

  return 0;
}


int IntersonManagerTest::TestSyncBulk_LineByLine_Overview()
{
  // Testing Receiving Data : B-Mode, Synchronous Bulk
  uInt8 motorSpeed10[] = {0xB9, 0x04, 0x00, 0x00, 0xB7, 0xA4, 0x32}; // 10.0 fps
  uInt8 motorSpeed15[] = {0x7A, 0x0D, 0x00, 0x00, 0x80, 0xA4, 0x32}; // 15.0 fps
  this->m_IntMng->initializeMotorSpeed(motorSpeed15);
  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_NONE);
  this->m_IntMng->startAcquisitionRoutine(0);
  std::cout<<std::dec;

  int r;
  int nRows = 241;
  int fps = 10;
  int sec = 1;
  int frameNumber = -1;
  int linesPerFrame = 0;
  for (int i = 0 ; i < nRows*fps*sec ; ++i)
  {
    int transferSize = 1024;
    unsigned char endpoint = 0x82;
    unsigned char * buffer = new unsigned char[transferSize];
    int transferred = 1;
    int BULK_TRANSFER_TIMEOUT = 1000;

    libusb_device_handle * handle = this->m_IntMng->getHandle();
    r = libusb_bulk_transfer(handle, endpoint, buffer, transferSize, &transferred, BULK_TRANSFER_TIMEOUT);


    // Terminal Max Size
    int maxTermRow = 25;
    int maxTermColumn = 25;

    // Overview of the frame
    if(i%(nRows/maxTermRow) == 0)
    {

      if(buffer[0] != frameNumber)
      {
        frameNumber = buffer[0];
        std::cout<<std::endl;
      }

      // Overview of the line
      for(int k = 0 ; k < transferSize ; k+= transferSize/maxTermColumn)
      {
        std::cout<<+buffer[k]<<" ";
      }
      std::cout<<std::endl;

      // Lines header
//      for(int k = 0 ; k < maxTermColumn ; k+= 1)
//      {
//        std::cout<<+buffer[k]<<" ";
//      }
//      std::cout<<std::endl;
    }
  }

  this->m_IntMng->stopAcquisitionRoutine();

  return 0;
}


int IntersonManagerTest::TestSyncBulk_LineByLine_CheckFrameNumbers()
{
  // Testing Receiving Data : B-Mode, Synchronous Bulk
  uInt8 motorSpeed10[] = {0xB9, 0x04, 0x00, 0x00, 0xB7, 0xA4, 0x32}; // 10.0 fps
  uInt8 motorSpeed15[] = {0x7A, 0x0D, 0x00, 0x00, 0x80, 0xA4, 0x32}; // 15.0 fps
  this->m_IntMng->initializeMotorSpeed(motorSpeed15);
  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_NONE);
  this->m_IntMng->startAcquisitionRoutine(0);
  std::cout<<std::dec;

  int r;
  int nRows = 241;
  int fps = 10;
  int sec = 3;
  int frameNumber = 0;
  int linesPerFrame = 0;
  for (int i = 0 ; i < nRows*fps*sec ; ++i)
  {
    int transferSize = 1024;
    unsigned char endpoint = 0x82;
    unsigned char * buffer = new unsigned char[transferSize];
    int transferred = 1;
    int BULK_TRANSFER_TIMEOUT = 1000;

    libusb_device_handle * handle = this->m_IntMng->getHandle();
    r = libusb_bulk_transfer(handle, endpoint, buffer, transferSize, &transferred, BULK_TRANSFER_TIMEOUT);

    linesPerFrame++;

    if(buffer[0] != frameNumber)
    {
      frameNumber = buffer[0];
      if(frameNumber <= 15 /*&& linesPerFrame > 10*/)
      {
//        std::cout<<frameNumber<<":"<<linesPerFrame<<" ";
        std::cout<<"frame # "<<frameNumber<<" : "<<linesPerFrame<<" lines";
        std::cout << std::endl;
      }
      linesPerFrame = 0;
    }
  }
  this->m_IntMng->stopAcquisitionRoutine();

  return 0;
}


int IntersonManagerTest::TestSyncBulk_LineByLine_CheckLengths()
{
  // Testing Receiving Data : B-Mode, Synchronous Bulk
  uInt8 motorSpeed10[] = {0xB9, 0x04, 0x00, 0x00, 0xB7, 0xA4, 0x32}; // 10.0 fps
  uInt8 motorSpeed15[] = {0x7A, 0x0D, 0x00, 0x00, 0x80, 0xA4, 0x32}; // 15.0 fps
  this->m_IntMng->initializeMotorSpeed(motorSpeed15);
  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_NONE);
  std::cout<<std::dec;

  this->m_IntMng->startAcquisitionRoutine(0);

  int r;
  int nRows = 241;
  for (int i = 0; i < 241*2; ++i) // Ask for 2 frames
  {
    int transferSize = 1024;
    unsigned char endpoint = 0x82;
    unsigned char * buffer = new unsigned char[transferSize];
    int transferred = 1;
    unsigned int BULK_TRANSFER_TIMEOUT = 1000;

    libusb_device_handle * handle = this->m_IntMng->getHandle();
    r = libusb_bulk_transfer(handle, endpoint, buffer, transferSize, &transferred, BULK_TRANSFER_TIMEOUT);

    // Looking for last non-zero value
    int lnz = transferSize;
    for(int j = transferSize ; j >= 0 ; j--)
    {
      if (buffer[j] != 0)
      {
        lnz = j;
        break;
      }
    }

    std::cout<<"i="<<i<<", r="<<r<<", t= "<<transferred<<", lnz="<<lnz<<std::endl;

    // Lines header
//      for(int k = 0 ; k < 5 ; k+= 1)
//      {
//        std::cout<<+buffer[k]<<" "<<std::endl;
//      }
//      std::cout<<std::endl;
//    }
  }

  this->m_IntMng->stopAcquisitionRoutine();

  return 0;
}
