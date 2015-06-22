//
// Created by Matthieu Heitz on 6/16/15.
// Copyright (c) 2015 Kitware Inc. All rights reserved.
//

#ifndef CYPRESSMANAGER_INTERSONMANAGERTEST_H
#define CYPRESSMANAGER_INTERSONMANAGERTEST_H


#include "IntersonManager.h"

class IntersonManagerTest
{
public:

  // Constructor, Destructor
  IntersonManagerTest(IntersonManager * manager);
  ~IntersonManagerTest();

  void printDevices(libusb_device ** deviceList);

  int TestHardButton();

  int TestMotor1Speed();

  int TestMotor3Speeds();

  int TestAcquisition();

  int TestSyncBulk_FrameByFrame_Overview();

  int TestSyncBulk_FrameByFrame_CheckFrameNumbers();

  int TestSyncBulk_LineByLine_Overview();

  int TestSyncBulk_LineByLine_CheckFrameNumbers();

  int TestSyncBulk_LineByLine_CheckLengths();


private:
  IntersonManager *m_IntMng;

};


#endif //CYPRESSMANAGER_INTERSONMANAGERTEST_H
