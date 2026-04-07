#ifndef __WINCHCONTROL_H__
#define __WINCHCONTROL_H__

#include <Arduino.h>
#include "DataSource.h"
#include "MotorDriver.h"

// Winch finite state machine states
enum WinchState {
  WINCH_IDLE,       // waiting at surface, not doing anything
  WINCH_DEPLOYING,  // lowering sensor line
  WINCH_COLLECTING, // holding at depth, collecting data
  WINCH_RETRACTING  // raising sensor line back up
};

class WinchControl : public DataSource
{
public:
  WinchControl(void);

  // Initialize with timing parameters (all in milliseconds)
  void init(int deployTime_ms, int collectTime_ms, int retractTime_ms,
            int deployPower, int retractPower);

  // Call every loop cycle — runs the deploy/collect/retract state machine
  // Returns true when the full cycle (deploy→collect→retract) is complete
  bool update(int currentTime);

  // Start a new deploy-collect-retract cycle
  void startCycle(void);

  // Motor C effort output — read this and pass to motor_driver
  int uV;

  // Current state (readable from main sketch)
  WinchState winchState;
  bool cycleComplete;

  String printString(void);

  // from DataSource
  size_t writeDataBytes(unsigned char * buffer, size_t idx);

  int lastExecutionTime = -1;

private:
  int _deployTime;    // how long to run motor down [ms]
  int _collectTime;   // how long to hold at depth [ms]
  int _retractTime;   // how long to run motor up [ms]
  int _deployPower;   // motor PWM for deploying (positive = down)
  int _retractPower;  // motor PWM for retracting (negative = up)
  int _cycleStartTime;
  int _stateStartTime;
};

#endif
