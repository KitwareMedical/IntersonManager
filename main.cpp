//
// Created by Matthieu Heitz on 6/9/15.
// Copyright (c) 2015 Kitware Inc. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "libusb.h"
#include "IntersonManager.h"
#include "IntersonManagerTest.h"


/** GLOBAL VARIABLES **/

IntersonManager * IntMng;
IntersonManagerTest * Tester;

// Number of frames to acquire
const int nFrames = 15;
// Buffer to keep all the frames acquired, to display them after acquisition.
// Used only when using the Memcopy callback.
unsigned char myBuffer[nFrames][241*1024];
int frameCount;

// Motor speed (used to set the speed and calculate the transfer timeouts
float fps = 15;
// Timeout of the handle_event(_timeout_completed) function in the event loop
// Used only for async-multithread implementation
int eventLoopTimeout_usec = static_cast<int>( 2e5 );
// Sleep time between transfer submissions.
// Used only for async-multithread implementation
int acquisitionLoopTimeout_usec = static_cast<int>( 1e6/fps*0.985 );

// Global variable used to stop the event loop.
// Used only for async-multithread implementation  int transferred = 1;
bool event_thread_run = true;
// Thread that executes the event loop.
// Used only for async-multithread implementation
pthread_t elt;


/** FUNCTION PROTOTYPES **/

// The difference between these two callback functions is the time they require to execute. Memcopy callback is
// in theory faster than Display Callback. The time difference isn't worth having when executing on Linux,
// but it might when executing on a tablet.

// This callback function memcopies the buffer and exits, so that we spent the least time
// in the callback function. The buffers content can be displayed by a loop in main(),
// after the recording
void TestAsyncBulk_FrameByFrame_CallbackMemCopy(struct libusb_transfer * transfer);

// This callback function can display an overview of it, or the frame numbers
// It obviously takes a little more time than memcopy callback.
void TestAsyncBulk_FrameByFrame_CallbackDisplay(struct libusb_transfer * transfer);

// This function uses the asynchronous libusb API, but use it in a synchronous way, i.e. without multihread.
// That's why I call it the pseudo-async implementation.
int TestAsyncBulk_FrameByFrame();

// This function uses the async libusb API, and works in multithread, with an event loop.
// I call it the async-multithread implementation.
void TestAsyncBulk_FrameByFrame_MultiThread();

// This is the event loop for the async-multithread implementation
void * EventHandlingLoop(void *);


/** MAIN **/

int main( int argc, char ** argv)
{

  IntMng = new IntersonManager();
  Tester = new IntersonManagerTest(IntMng);
  //libusb_device_handle * handle = IntMng->getHandle();

//  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);
  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_NONE);
//  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_ERROR);
//  libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_INFO);

//  libusb_set_configuration(handle, 1);
//  libusb_claim_interface(handle, 0);

  libusb_set_configuration(IntMng->getHandle(), 1);
  libusb_claim_interface(IntMng->getHandle(), 0);


//  Tester->TestSyncBulk_FrameByFrame_Overview();
//  Tester->TestSyncBulk_FrameByFrame_CheckFrameNumbers();

//  Tester->TestSyncBulk_LineByLine_Overview();
//  Tester->TestSyncBulk_LineByLine_CheckFrameNumbers();
//  Tester->TestSyncBulk_LineByLine_CheckLengths();

//  TestAsyncBulk_FrameByFrame(); // pseudo-async
  TestAsyncBulk_FrameByFrame_MultiThread();


  std::cout<<"Done"<<std::endl;


//  libusb_release_interface(handle, 0);
//  libusb_close(handle);

  pthread_join(elt, NULL);

  // Check memcopied buffers. To be used if you set the callback
  // to TestAsyncBulk_FrameByFrame_CallbackMemCopy
  std::cout << "myBuffer Frame numbers:" << std::hex << std::endl;
  for (int i = 0; i < nFrames; ++i)
  {
    for (int k = 0; k < 241 * 1024; k += 1024)
    {
      std::cout << +myBuffer[i][k] << " ";
    }
    std::cout << std::hex << std::endl << std::endl;
  }

  //libusb_free_device_list(deviceList, 1);

//  libusb_exit(NULL);
  delete Tester;
  delete IntMng;
  return 0;
}



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

  // Terminal max size (used to downsize a frame to the size of the terminal used)
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
  unsigned int BULK_TRANSFER_TIMEOUT = 1000;
  libusb_transfer * asyncBulk = libusb_alloc_transfer(0);

  libusb_transfer_cb_fn callbackFunction;
  callbackFunction = &TestAsyncBulk_FrameByFrame_CallbackDisplay;
//  callbackFunction = &TestAsyncBulk_FrameByFrame_CallbackMemCopy;

  libusb_fill_bulk_transfer(asyncBulk, IntMng->getHandle(), endpoint, buffer, transferSize,
                            callbackFunction, NULL, BULK_TRANSFER_TIMEOUT);

  IntMng->startAcquisitionRoutine(0);

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
    // 2e5 is a good value

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
//    libusb_handle_events(NULL);
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = eventLoopTimeout_usec;
    libusb_handle_events_timeout_completed(NULL, &tv, NULL);
    std::cout<<"Event handled"<<std::endl;
  }
  std::cout<<"Event handling loop: END"<<std::endl;
  return NULL;
}


void TestAsyncBulk_FrameByFrame_MultiThread()
{
  std::cout << std::dec;
  int transferSize = 241 * 1024;
  unsigned char endpoint = 0x82;
  unsigned char *buffer = new unsigned char[transferSize];
  unsigned int BULK_TRANSFER_TIMEOUT = 1000;
  libusb_transfer * asyncBulk = libusb_alloc_transfer(0);

  libusb_transfer_cb_fn callbackFunction;
//  callbackFunction = &TestAsyncBulk_FrameByFrame_CallbackDisplay;
  callbackFunction = &TestAsyncBulk_FrameByFrame_CallbackMemCopy;

  libusb_fill_bulk_transfer(asyncBulk, IntMng->getHandle(), endpoint, buffer, transferSize,
                            callbackFunction, NULL, BULK_TRANSFER_TIMEOUT);

  pthread_create(&elt, NULL, EventHandlingLoop, NULL);

  IntMng->setMotorSpeed(fps);
  std::cout<<"Start acquisition"<<std::endl;
  IntMng->startAcquisitionRoutine(0);

  // These waiting times are relevant only when working in async multithread mode (this function)
  // In other modes (synchronous, or pseudo-async), there shouldn't be any sleeping time here, otherwise,
  // it will make the frame numbers starts later than 0. (2~3 for usleep(2e5);)

  // For all these waiting times, the data has glitches. Sometimes it doesn't, but I can observe a shift in the line
  // i.e. I don't receive 241 lines with the same frame number, but less.
  // I call "glitches" when the frame numbers (1st value of each scan line (1024 values)) are numbers
  // different from (0-15), which means that there is a shift inside the lines themselves.
  // I call a "shift" in the frame numbers when the frame numbers are consistent (between 0 and 15),
  // but in one total frame, there isn't only one frame number value. Usually, this shift is propagated
  // in all the frames.
  // The shift can be constant: the index where the frame number changes in a frame, is the same across
  // all the frames. This means that no data is lost -> for each frame number, 241 lines are received.
  // The shift can be varying: the index where the frame number changes in a frame, is not the same across
  // all the frames. This means that some data is lost -> for each frame number, less than 241 lines are received.

  // Sufficient to not have the resource busy error, but frame count doesn't start at 0
  usleep(2e5);

  // Very sufficient to not have the resource busy error, but frame count starts at 0xF.
//  usleep(1e6);

  // Not sufficient: the first frame always return resource busy. But the next one starts with 0
//  usleep(66.6e3);

  // Sufficient to not have the resource busy error, but frame starts at 1
//  usleep(1.33e5);

  // Make the resource busy error sometimes. when it doesn't frame count starts at 1.
//  usleep(1e5);

  // Sufficient to not have the resource busy error, frame starts at 0xf or 0, it depends...
//  usleep(1e6*16/15);

  // TODO : First, fix the glitch problems, then the shift problems, because when there are glitches,
  // it is very hard to debug the shifting problem.


  int r;

  for(frameCount = 0 ; frameCount < nFrames ; ++frameCount)
  {
    std::cout<<"FrameCount: "<<frameCount<<std::endl;
    asyncBulk->user_data = &frameCount;
    r = libusb_submit_transfer(asyncBulk);
    IntMng->Error(r);
    usleep(acquisitionLoopTimeout_usec);
    // Values of acquisitionLoopTimeout_usec:
    //
    // 1e6/fps: Has glitches
    //
    // 1e6/fps*0.98: Has glitches less often, and lose data less often, but still not perfect.
    // Make the "resource busy" error very rarely.
    //
    // 1e6/fps*0.97: Has glitches rarely, but make the "resource busy" error appear often (because of
    // resubmitting a transfer when the previous one hasn't been finished)
    //
    // The observations are based on a lot of tries, because the process is quite random.
  }


  std::cout<<"Stop acquisition"<<std::endl;
  IntMng->stopAcquisitionRoutine();

  event_thread_run = false;
}
