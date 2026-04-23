#include "WinchControl.h"
#include "Printer.h"
extern Printer printer;

WinchControl::WinchControl(void)
: DataSource("uV_winch,winch_state","int,int") {}

void WinchControl::init(int deployTime_ms, int collectTime_ms, int retractTime_ms,
                         int deployPower, int retractPower) {
  _deployTime   = deployTime_ms;
  _collectTime  = collectTime_ms;
  _retractTime  = retractTime_ms;
  _deployPower  = deployPower;
  _retractPower = retractPower;
  winchState    = WINCH_IDLE;
  cycleComplete = false;
  uV = 0;
}

void WinchControl::startCycle(void) {
  winchState    = WINCH_DEPLOYING;
  cycleComplete = false;
  _stateStartTime = millis();
  printer.printMessage("Winch: Starting deploy", 10);
}

bool WinchControl::update(int currentTime) {
  switch (winchState) {

    case WINCH_IDLE:
      uV = 0;
      break;

    case WINCH_DEPLOYING:
      uV = _deployPower;  // positive = lower line
      if (currentTime - _stateStartTime >= _deployTime) {
        winchState = WINCH_COLLECTING;
        _stateStartTime = currentTime;
        uV = 0;
        printer.printMessage("Winch: At depth, collecting data", 10);
      }
      break;

    case WINCH_COLLECTING:
      uV = 0;  // hold position, sensors are reading
      if (currentTime - _stateStartTime >= _collectTime) {
        winchState = WINCH_RETRACTING;
        _stateStartTime = currentTime;
        printer.printMessage("Winch: Retracting line", 10);
      }
      break;

    case WINCH_RETRACTING:
      uV = _retractPower;  // negative = raise line
      if (currentTime - _stateStartTime >= _retractTime) {
        winchState = WINCH_IDLE;
        uV = 0;
        cycleComplete = true;
        printer.printMessage("Winch: Cycle complete, line retracted", 10);
      }
      break;
  }

  return cycleComplete;
}

String WinchControl::printString(void) {
  String s = "Winch: ";
  switch (winchState) {
    case WINCH_IDLE:       s += "IDLE";       break;
    case WINCH_DEPLOYING:  s += "DEPLOYING";  break;
    case WINCH_COLLECTING: s += "COLLECTING"; break;
    case WINCH_RETRACTING: s += "RETRACTING"; break;
  }
  s += " uV=" + String(uV);
  return s;
}

size_t WinchControl::writeDataBytes(unsigned char * buffer, size_t idx) {
  int * data_slot = (int *) &buffer[idx];
  data_slot[0] = uV;
  data_slot[1] = (int)winchState;
  return idx + 2 * sizeof(int);
}
