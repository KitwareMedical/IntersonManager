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

  char c;
  bool bTask = true;

  while(bTask)
  {
    if(kbhit())
    {
        c = getchar();
        if(c == 'q')
            bTask = false;
        else if(c == 'd')
        {
            std::cout << ">>>display" << std::endl;
            IntMng.PrintDevice();
        }
        else if (c == 'c')
        {
            int Freqs[6] = {0, 1, 2, 3, 4, 5};
            int powers[6] = {10,20,30,40,50,60};

//            int numberOfFrequency = (sizeof(Freqs)/sizeof(int));
//            int numberOfPower     = (sizeof(powers)/sizeof(int));

//            std::cout << numberOfFrequency << std::endl;
//            std::cout << numberOfPower << std::endl;

            for(int i=0; i<30; ++i)
            {
                IntMng.AcquireUSspectroscopyFrames(Freqs, powers);
            }
        }
    }
  }

  return 0;
}
