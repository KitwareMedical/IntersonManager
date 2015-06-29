//
// Created by Matthieu Heitz on 6/9/15.
// Copyright (c) 2015 Kitware Inc. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <ctime>
//#include <thread>
//#include <pthread.h>
#include <signal.h>
#include <string.h>

#include "libusb.h"
#include "IntersonManager.h"
#include "IntersonManagerTest.h"



IntersonManager * IntMng;
IntersonManagerTest * Tester;

const int nFrames = 15;
unsigned char myBuffer[nFrames][241*1024];
int frameCount;

float fps = 15;
int eventLoopTimeout_usec = static_cast<int>( 1e6/fps/2 );
int acquisitionLoopTimeout_usec = static_cast<int>( 1e6/fps );

bool event_thread_run = true;
pthread_t elt;
pthread_mutex_t * thread_run_mutex;



void TestAsyncBulk_FrameByFrame_CallbackMemCopy(struct libusb_transfer * transfer)
{
  std::cout<<"MemCopy Callback"<<std::endl;
  if(transfer->actual_length == transfer->length)
  {
    std::cout<<"Copying Buffer"<<std::endl;
    memcpy(myBuffer[frameCount], transfer->buffer, transfer->actual_length);
  }
}


void TestAsyncBulk_FrameByFrame_CallbackDisplay(struct libusb_transfer * transfer)
{
  std::cout<<"Display Callback"<<std::endl;

  std::cout << std::dec;
  std::cout << "Status: " << transfer->status << std::endl;
  std::cout << "Length: " << transfer->length << ", Actual: " << transfer->actual_length << std::endl;
//  std::cout << "UserData: " << *static_cast<int *>(transfer->user_data) << std::endl;

  // Frame size
  int nRows = 241;
  int nColumn = 1024;

  // Terminal max size
  int maxTermRow = 25;
  int maxTermColumn = 25;

//  // Overview of the frame
//  if(transfer->actual_length == transfer->length)
//  {
//    for (int k = 0; k < nRows * nColumn; k += nColumn * (nRows / maxTermRow))
//    {
//      for (int j = 0; j < nColumn; j += nColumn / maxTermColumn)
//      {
//        std::cout << +transfer->buffer[k + j] << " ";
//      }
//      std::cout << std::endl;
//    }
//    std::cout << std::endl;
//  }

  // Check Frame numbers
  if(transfer->actual_length == transfer->length)
  {
    std::cout << "Frame numbers:" << std::hex << std::endl;
    for (int k = 0; k < transfer->actual_length; k += 1024)
    {
      std::cout << +transfer->buffer[k] << " ";
    }
    std::cout << std::dec << std::endl;
  }
}

int TestAsyncBulk_FrameByFrame()
{
  std::cout << std::dec;
  int transferSize = 241 * 1024;
  unsigned char endpoint = 0x82;
  unsigned char *buffer = new unsigned char[transferSize];
  int transferred = 1;
  unsigned int BULK_TRANSFER_TIMEOUT = 1000;
  libusb_transfer * asyncBulk = libusb_alloc_transfer(0);

  libusb_transfer_cb_fn callbackFunction;
  callbackFunction = &TestAsyncBulk_FrameByFrame_CallbackDisplay;
//  callbackFunction = &TestAsyncBulk_FrameByFrame_CallbackMemCopy;

  libusb_fill_bulk_transfer(asyncBulk, IntMng->getHandle(), endpoint, buffer, transferSize,
                            callbackFunction, NULL, BULK_TRANSFER_TIMEOUT);

  IntMng->startAcquisitionRoutine(0);

  int i;
  for(frameCount = 0 ; frameCount < nFrames ; ++frameCount)
  {
    std::cout << "Submitting transfer " << frameCount << std::endl;
//    asyncBulk->user_data = &frameCount;
    int r = libusb_submit_transfer(asyncBulk);
    IntMng->Error(r);

    // FOR LINUX:
    // 1st method: Works very well ! Using CallbackDisplay or CallbackMemcopy doesn't change anything
    //             Problem is that this function is deprecated and API asks to use one of the two others
    // 2nd and 3rd method: When timeout = 1e6/fps, or 1e6/fps*2, Segfault 50% of time. This is because the first
    // frames take more time to come, so if the timeout isn't big enough, the application will crash,
    // because it resubmits another transfer that hasn't been completed yet.
    // 1e6/fps*2 is 133333 (1.3e5), and it isn't sufficient, 2e5 is apparently sufficient, but we can set 5e5 to be sure.
    // In theory, we don't have to wait for the data to arrive, the probe delivers it no matter what.
    // For another device, it would be the maximum time in which we expect the transfer to be done.
    // 1.8e5 works always, 1.7e5 makes segfaults. 2e5 is a good value. 5e5 is a little high

    // FOR ANDROID:
    //

    // 1st method
//    libusb_handle_events(NULL);
    // 2nd method
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = static_cast<int>( 2e5 );
    libusb_handle_events_timeout_completed(NULL, &tv, NULL);
    // 3rd method
//    timeval tv;
//    tv.tv_sec = 0;
//    tv.tv_usec = static_cast<int>( 2e5 );
//    libusb_handle_events_timeout(NULL, &tv);

    std::cout << "Events handled" << std::endl;
  }
  std::cout<<"Freeing transfer"<<std::endl;
  libusb_free_transfer(asyncBulk);
  delete buffer;

  IntMng->stopAcquisitionRoutine();


  return 0;
}





void * EventHandlingLoop(void *)
{
  std::cout<<"Event handling loop: START"<<std::endl;
  while (event_thread_run)
  {
    libusb_handle_events(NULL);
//    usleep(eventLoopTimeout_usec);
//    timeval tv;
//    tv.tv_sec = 0;
//    tv.tv_usec = eventLoopTimeout_usec;
//    libusb_handle_events_timeout_completed(NULL, &tv, NULL);
//    std::cout<<"Event handled"<<std::endl;
  }
  std::cout<<"Event handling loop: END"<<std::endl;
  return NULL;

//  clock_t t0 = clock();
//  std::cout<<"Start Event Handling loop"<<std::endl;
//  while( (clock() - t0) < 5e6 )
//  {
////    std::cout<<(clock() - t0)<<std::endl;
////    libusb_handle_events(NULL);
//  }
//  std::cout<<"Stop Event Handling loop"<<std::endl;
//  pthread_exit(NULL);
}


void TestAsyncBulk_FrameByFrame_MultiThread()
{
  std::cout << std::dec;
  int transferSize = 241 * 1024;
  unsigned char endpoint = 0x82;
  unsigned char *buffer = new unsigned char[transferSize];
  int transferred = 1;
  unsigned int BULK_TRANSFER_TIMEOUT = 1000;
  libusb_transfer * asyncBulk = libusb_alloc_transfer(0);

  libusb_transfer_cb_fn callbackFunction;
  callbackFunction = &TestAsyncBulk_FrameByFrame_CallbackDisplay;
//  callbackFunction = &TestAsyncBulk_FrameByFrame_CallbackMemCopy;

  libusb_fill_bulk_transfer(asyncBulk, IntMng->getHandle(), endpoint, buffer, transferSize,
                            callbackFunction, NULL, BULK_TRANSFER_TIMEOUT);

  pthread_create(&elt, NULL, EventHandlingLoop, NULL);

  IntMng->setMotorSpeed(fps);
  std::cout<<"Start acquisition"<<std::endl;
  IntMng->startAcquisitionRoutine(0);

  // Sufficient to not have the resource busy error, but frame count doesn't start at 0
  usleep(2e5);

  int r;

  for(frameCount = 0 ; frameCount < nFrames ; ++frameCount)
  {
    std::cout<<"FrameCount: "<<frameCount<<std::endl;
    asyncBulk->user_data = &frameCount;
    r = libusb_submit_transfer(asyncBulk);
    IntMng->Error(r);
    usleep(acquisitionLoopTimeout_usec);
  }


  std::cout<<"Stop acquisition"<<std::endl;
  IntMng->stopAcquisitionRoutine();

  event_thread_run = false;
}




int main( int argc, char ** argv)
{
  int r;
  r = libusb_init(NULL);
  if (r < 0)
  {
    return r;
  }

  libusb_device ** deviceList;
  ssize_t cnt;
  cnt = libusb_get_device_list(NULL, &deviceList);
  if (cnt < 0)
    return (int) cnt;
  std::cout<<"Number of devices = "<<cnt<<std::endl;


  libusb_device_handle * handle = libusb_open_device_with_vid_pid(NULL, 0x1921, 0x0090);
  if(!handle)
  {
    std::cerr<<"Impossible to open device 1921:0090"<<std::endl;
    return -1;
  }

  IntMng = new IntersonManager(handle);
  Tester = new IntersonManagerTest(IntMng);

//  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);
  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_NONE);
//  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_ERROR);
//  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_INFO);

  libusb_set_configuration(handle, 1);
  libusb_claim_interface(handle, 0);


//  Tester->TestSyncBulk_FrameByFrame_Overview();
//  Tester->TestSyncBulk_FrameByFrame_CheckFrameNumbers();

//  Tester->TestSyncBulk_LineByLine_Overview();
//  Tester->TestSyncBulk_LineByLine_CheckFrameNumbers();
//  Tester->TestSyncBulk_LineByLine_CheckLengths();

//  TestAsyncBulk_FrameByFrame();
  TestAsyncBulk_FrameByFrame_MultiThread();


  std::cout<<"Done"<<std::endl;


  libusb_release_interface(handle, 0);
  libusb_close(handle);

  pthread_join(elt, NULL);

//  // Check memcopied buffers. Uncomment if you set the callback
//  // to TestAsyncBulk_FrameByFrame_CallbackMemCopy
//  std::cout << "myBuffer Frame numbers:" << std::hex << std::endl;
//  for (int i = 0; i < nFrames; ++i)
//  {
//    for (int k = 0; k < 241 * 1024; k += 1024)
//    {
//      std::cout << +myBuffer[i][k] << " ";
//    }
//    std::cout << std::hex << std::endl << std::endl;
//  }

  libusb_free_device_list(deviceList, 1);

  libusb_exit(NULL);
  return 0;
}







