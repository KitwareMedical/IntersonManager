//
// Created by Hyun Jae Kang
// Copyright (c) 2015 Kitware Inc. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include "IntersonManager.h"
#include "IntersonManagerTest.h"
#include <fstream>

libusb_device_handle * m_USBHandle;

int kbhit()
{
#if defined(WIN32) || defined (_WIN32) || defined (__WIN32__)
    return _kbhit();
#else
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF)
    {
    ungetc(ch, stdin);
    return 1;
    }

    return 0;
#endif
}

/** MAIN **/
int main( int argc, char ** argv)
{
  // Create an instance of IntersonManager
  IntersonManager IntMng;
  IntMng.setVerbose(false);
  //m_USBHandle = IntMng.getHandle();

  //IntersonManager* IntMng2 = new IntersonManager(m_USBHandle);

  char c;
  bool bTask = true;

  while(bTask)
  {
    if(kbhit())
    {
      c = getchar();
      if(c == 'q')
        {
        bTask = false;
        }
      else if (c == 'c')
        {
        std::list<int> Freqs;
        Freqs.clear();
        Freqs.push_back(0);
        Freqs.push_back(1);
        Freqs.push_back(2);

        std::list<int> powers;
        powers.clear();
        powers.push_back(10);
        powers.push_back(20);

        IntMng.AcquireUSspectroscopyFrames(Freqs, powers);
        }
      else if(c == 'r')
        {
        int FrameSize = 241*2048;
        short* buffer = new short[FrameSize];
        int DataSize = FrameSize*sizeof(short);

        IntMng.initializeProbeRFMode();

        //uInt8 motorSpeed10[] = {0xB9, 0x04, 0x00, 0x00, 0xB7, 0xA4, 0x32}; // 10.0 fps
        uInt8 motorSpeed15[] = {0x7A, 0x0D, 0x00, 0x00, 0x80, 0xA4, 0x32}; // 15.0 fps
        IntMng.initializeMotorSpeed(motorSpeed15);
        IntMng.sendHighVoltage(100);
        IntMng.startAcquisitionRoutine(1); //RF-Data
        //usleep(6.7e5);
        IntMng.GetNewFrame((unsigned char*)buffer, DataSize);
        IntMng.stopAcquisitionRoutine();

        std::ofstream outfile("RFData.raw", std::ofstream::binary);
        outfile.write((char*)buffer, DataSize);
        outfile.close();

        delete buffer;
        buffer = NULL;
        }
      else if (c == 'b')
        {
        int nRows = 1024;
        int nColumn = 241;
        int FrameSize = 241*1024;
        unsigned char* Bbuffer = new unsigned char[FrameSize];
        int DataSize = FrameSize*sizeof(unsigned char);

        IntMng.initializeProbe();
        IntMng.setMotorSpeed(15.0f);
        IntMng.sendHighVoltage(100);
        IntMng.startAcquisitionRoutine(0); //B-Mode
        IntMng.GetNewFrame((unsigned char*)Bbuffer, DataSize);


        // Terminal max size
        int maxTermRow = 25;
        int maxTermColumn = 25;

        // Overview of the frame
        for (int k = 0; k < nRows * nColumn; k += nColumn*(nRows/maxTermRow))
          {
          for (int j = 0; j < nColumn; j+= nColumn/maxTermColumn)
            {
            std::cout << +Bbuffer[k+j] << " ";
            }
          std::cout << std::endl;
          }
        std::cout << std::endl;

        std::ofstream outfile("BModeData.raw", std::ofstream::binary);
        outfile.write((char*)Bbuffer, DataSize);
        outfile.close();

        IntMng.stopAcquisitionRoutine();
        delete Bbuffer;
        Bbuffer = NULL;
        }
    }

  }


  IntMng.CloseLibUSB();
  //delete m_USBHandle;
//  delete IntMng2;
  return 0;
}
