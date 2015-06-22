//
// Created by Matthieu Heitz on 6/10/15.
// Copyright (c) 2015 Kitware Inc. All rights reserved.
//

#ifndef CYPRESSMANAGER_INTERSONMANAGER_H
#define CYPRESSMANAGER_INTERSONMANAGER_H

#include <iostream>
#include <libusb.h>

typedef uint8_t uInt8;
typedef uint16_t uInt16;
typedef uint32_t uInt32;


class IntersonManager
{

public:

  IntersonManager(libusb_device_handle * handle);

  // Interson-specific control requests

  bool initializeAnalogTGC(uInt8 TGC[3]);
  bool initializeDigitalTGC(uInt8 TGC[10]);
  bool initializeMotorSpeed(uInt8 speedArray[7]);
  bool initializeSamplerInc();
  bool readFPGAVersion(uInt8 * data);
  bool readHardButton(uInt8 * data);
  bool readOEMSpace(int address, int length, uInt8 * data);
  bool sendHighVoltage(int highVoltage);
  bool setDynamicRange();
  bool setFrequencyBandPassFilter(int frequencyIndex);
  bool setFrequencyIndex(int frequencyIndex);
  bool setFrequencyInit();
  bool setEnableHighVoltage(bool enable);
  bool setEnableRFDecimator(bool enable);
  bool setMotorSpeed(float fps);
  bool setStartMotor(bool start);
  bool startBMode();
  bool startRFMode();
  bool stopAcquisition();
  bool writeOEMSpace(int address, int length, uInt8 * data);

  // Routine and helper functions
  void Error(int err);
  libusb_device_handle * getHandle() const;
  bool initializeProbe();
  void setVerbose(bool verbose);
  bool startAcquisitionRoutine(int acquisitionMode);
  bool stopAcquisitionRoutine();

private:

  libusb_device_handle * m_Handle;
  static const int VENDORCMD_TIMEOUT = 1000;
  bool m_Verbose;
  int m_AcquisitionMode;

};


#endif //CYPRESSMANAGER_INTERSONMANAGER_H
