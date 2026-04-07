/********
E80 Final Project — Kelp Forest Grid Survey
Team 48: GPS-guided grid navigation with automatic winch deploy/retract

State Machine:
  NAVIGATE  → drive to next grid waypoint using SurfaceControl (Motor A + B)
  DEPLOY    → lower sensor line via winch (Motor C)
  COLLECT   → hold at depth while sensors log PAR, pressure, temperature
  RETRACT   → raise sensor line back up
  ADVANCE   → move to next waypoint, repeat

Authors: Steve
********/

#include <Arduino.h>
#include <Wire.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include <Pinouts.h>
#include <TimingOffsets.h>
#include <SensorGPS.h>
#include <SensorIMU.h>
#include <XYStateEstimator.h>
#include <ADCSampler.h>
#include <ErrorFlagSampler.h>
#include <ButtonSampler.h>
#include <MotorDriver.h>
#include <Logger.h>
#include <Printer.h>
#include <SurfaceControl.h>
#include <WinchControl.h>
#define UartSerial Serial1
#include <GPSLockLED.h>

/////////////////////////* Global Variables *////////////////////////

MotorDriver    motor_driver;
XYStateEstimator xy_state_estimator;
SurfaceControl surface_control;
WinchControl   winch_control;
SensorGPS      gps;
Adafruit_GPS   GPS(&UartSerial);
ADCSampler     adc;
ErrorFlagSampler ef;
ButtonSampler  button_sampler;
SensorIMU      imu;
Logger         logger;
Printer        printer;
GPSLockLED     led;

// Loop timing
int loopStartTime;
int currentTime;
volatile bool EF_States[NUM_FLAGS] = {1, 1, 1};

/////////////////////////* Mission State Machine *////////////////////////

enum MissionState {
  MISSION_NAVIGATE,   // driving to next grid point
  MISSION_DEPLOY,     // lowering winch
  MISSION_COLLECT,    // holding at depth
  MISSION_RETRACT,    // raising winch
  MISSION_COMPLETE    // all grid points done
};

MissionState missionState = MISSION_NAVIGATE;
int currentGridPoint = 0;

/////////////////////////* Grid Waypoints *////////////////////////
// Grid layout near Dana Point Harbor, east of Breakwater Light 3
// Coordinates in LOCAL x,y meters relative to origin in XYStateEstimator.h
// Origin should be set to the GPS coordinates of your launch point
//
// Example: 3x3 grid with 10m spacing (adjust to your actual survey area)
//   Grid pattern (top view, North = +y, East = +x):
//
//     (0,20) -- (10,20) -- (20,20)
//       |         |          |
//     (0,10) -- (10,10) -- (20,10)
//       |         |          |
//     (0, 0) -- (10, 0) -- (20, 0)  ← start
//
// The robot traverses in a serpentine (boustrophedon) path to minimize travel:
//   Row 0 left→right, Row 1 right→left, Row 2 left→right

const int num_grid_waypoints = 9;
double grid_waypoints[] = {
  // x[m],  y[m]    — serpentine order
   0.0,   0.0,    // Grid (0,0) — start / launch point
  10.0,   0.0,    // Grid (1,0)
  20.0,   0.0,    // Grid (2,0)
  20.0,  10.0,    // Grid (2,1) — reverse direction
  10.0,  10.0,    // Grid (1,1)
   0.0,  10.0,    // Grid (0,1)
   0.0,  20.0,    // Grid (0,2) — reverse again
  10.0,  20.0,    // Grid (1,2)
  20.0,  20.0     // Grid (2,2) — final point
};

/////////////////////////* Winch Parameters *////////////////////////
// Tune these based on your winch gear ratio, line length, and motor speed
// All times in milliseconds

const int WINCH_DEPLOY_TIME_MS  = 15000;  // time to lower line to ~20ft (~6m)
const int WINCH_COLLECT_TIME_MS = 10000;  // time to hold at depth for data collection
const int WINCH_RETRACT_TIME_MS = 15000;  // time to raise line back up
const int WINCH_DEPLOY_POWER    = 60;     // Motor C PWM for lowering (positive = down)
const int WINCH_RETRACT_POWER   = -60;    // Motor C PWM for raising  (negative = up)

// Delay at each surface waypoint before starting winch (let robot stabilize)
const int STABILIZE_DELAY_MS    = 3000;
int stabilizeStartTime = 0;

////////////////////////* Setup *////////////////////////////////

void setup() {

  // Register all data sources with the logger
  logger.include(&imu);
  logger.include(&gps);
  logger.include(&xy_state_estimator);
  logger.include(&surface_control);
  logger.include(&winch_control);
  logger.include(&motor_driver);
  logger.include(&adc);
  logger.include(&ef);
  logger.include(&button_sampler);
  logger.init();

  printer.init();
  ef.init();
  button_sampler.init();
  imu.init();
  UartSerial.begin(9600);
  gps.init(&GPS);
  motor_driver.init();
  led.init();

  // Initialize surface control with the grid waypoints
  // navigateDelay = 0 because we handle delays in our own state machine
  surface_control.init(num_grid_waypoints, grid_waypoints, 0);

  // Initialize winch control
  winch_control.init(WINCH_DEPLOY_TIME_MS, WINCH_COLLECT_TIME_MS, WINCH_RETRACT_TIME_MS,
                     WINCH_DEPLOY_POWER, WINCH_RETRACT_POWER);

  xy_state_estimator.init();

  printer.printMessage("Team 48: Grid Survey Starting", 10);
  loopStartTime = millis();

  // Set timing offsets for each subsystem
  printer.lastExecutionTime            = loopStartTime - LOOP_PERIOD + PRINTER_LOOP_OFFSET;
  imu.lastExecutionTime                = loopStartTime - LOOP_PERIOD + IMU_LOOP_OFFSET;
  adc.lastExecutionTime                = loopStartTime - LOOP_PERIOD + ADC_LOOP_OFFSET;
  ef.lastExecutionTime                 = loopStartTime - LOOP_PERIOD + ERROR_FLAG_LOOP_OFFSET;
  button_sampler.lastExecutionTime     = loopStartTime - LOOP_PERIOD + BUTTON_LOOP_OFFSET;
  xy_state_estimator.lastExecutionTime = loopStartTime - LOOP_PERIOD + XY_STATE_ESTIMATOR_LOOP_OFFSET;
  surface_control.lastExecutionTime    = loopStartTime - LOOP_PERIOD + SURFACE_CONTROL_LOOP_OFFSET;
  winch_control.lastExecutionTime      = loopStartTime - LOOP_PERIOD + DEPTH_CONTROL_LOOP_OFFSET;
  logger.lastExecutionTime             = loopStartTime - LOOP_PERIOD + LOGGER_LOOP_OFFSET;
}

//////////////////////////////* Loop */////////////////////////

void loop() {
  currentTime = millis();

  //===================== PRINTER =====================
  if (currentTime - printer.lastExecutionTime > LOOP_PERIOD) {
    printer.lastExecutionTime = currentTime;
    printer.printValue(0, adc.printSample());
    printer.printValue(1, ef.printStates());
    printer.printValue(2, logger.printState());
    printer.printValue(3, gps.printState());
    printer.printValue(4, xy_state_estimator.printState());
    printer.printValue(5, surface_control.printWaypointUpdate());
    printer.printValue(6, surface_control.printString());
    printer.printValue(7, winch_control.printString());
    printer.printValue(8, motor_driver.printState());
    printer.printValue(9, imu.printRollPitchHeading());
    printer.printToSerial();
  }

  //===================== MISSION STATE MACHINE =====================
  if (currentTime - surface_control.lastExecutionTime > LOOP_PERIOD) {
    surface_control.lastExecutionTime = currentTime;

    switch (missionState) {

      // ---- STATE 1: Navigate to next grid point ----
      case MISSION_NAVIGATE:
        if (surface_control.navigateState) {
          if (!surface_control.atPoint) {
            // Still driving toward the waypoint
            surface_control.navigate(&xy_state_estimator.state, &gps.state, currentTime);
            motor_driver.drive(surface_control.uL, surface_control.uR, 0);
          }
          else if (surface_control.complete) {
            // All waypoints exhausted
            motor_driver.drive(0, 0, 0);
            missionState = MISSION_COMPLETE;
            printer.printMessage("Mission complete! All grid points surveyed.", 10);
          }
          else {
            // Arrived at a waypoint — stop motors and transition to deploy
            motor_driver.drive(0, 0, 0);
            stabilizeStartTime = currentTime;
            missionState = MISSION_DEPLOY;
            printer.printMessage("At grid point " + String(currentGridPoint) + ", stabilizing...", 10);
          }
        }
        break;

      // ---- STATE 2: Deploy winch (with brief stabilization delay) ----
      case MISSION_DEPLOY:
        motor_driver.drive(0, 0, 0);  // keep XY motors off

        if (currentTime - stabilizeStartTime >= STABILIZE_DELAY_MS) {
          // Start the winch cycle (deploy → collect → retract)
          winch_control.startCycle();
          missionState = MISSION_COLLECT;
          printer.printMessage("Deploying winch at grid point " + String(currentGridPoint), 10);
        }
        break;

      // ---- STATE 3 & 4: Winch cycle running (deploy/collect/retract handled internally) ----
      case MISSION_COLLECT:
      case MISSION_RETRACT:
      {
        // Update winch state machine — it handles deploy→collect→retract internally
        bool done = winch_control.update(currentTime);
        motor_driver.drive(0, 0, winch_control.uV);

        // Track sub-state for the mission enum (for logging clarity)
        if (winch_control.winchState == WINCH_RETRACTING) {
          missionState = MISSION_RETRACT;
        }

        if (done) {
          // Winch cycle complete — advance to next waypoint
          currentGridPoint++;
          winch_control.cycleComplete = false;
          surface_control.atPoint = false;  // tell SurfaceControl to move to next waypoint
          missionState = MISSION_NAVIGATE;
          printer.printMessage("Grid point " + String(currentGridPoint - 1) +
                               " done, navigating to next.", 10);
        }
        break;
      }

      case MISSION_COMPLETE:
        motor_driver.drive(0, 0, 0);
        // Mission is done — robot sits idle, logger keeps running
        break;
    }
  }

  //===================== ADC SAMPLING =====================
  // Reads all 9 ADC channels including your 3 sensors:
  //   A00 (pin 14) = pressure sensor
  //   A01 (pin 15) = thermistor
  //   A02 (pin 16) = PAR photodiode
  // (Assign your actual wiring to these pins — adjust in ADCSampler if needed)
  if (currentTime - adc.lastExecutionTime > LOOP_PERIOD) {
    adc.lastExecutionTime = currentTime;
    adc.updateSample();
  }

  //===================== ERROR FLAGS =====================
  if (currentTime - ef.lastExecutionTime > LOOP_PERIOD) {
    ef.lastExecutionTime = currentTime;
    attachInterrupt(digitalPinToInterrupt(ERROR_FLAG_A), EFA_Detected, LOW);
    attachInterrupt(digitalPinToInterrupt(ERROR_FLAG_B), EFB_Detected, LOW);
    attachInterrupt(digitalPinToInterrupt(ERROR_FLAG_C), EFC_Detected, LOW);
    delay(5);
    detachInterrupt(digitalPinToInterrupt(ERROR_FLAG_A));
    detachInterrupt(digitalPinToInterrupt(ERROR_FLAG_B));
    detachInterrupt(digitalPinToInterrupt(ERROR_FLAG_C));
    ef.updateStates(EF_States[0], EF_States[1], EF_States[2]);
    EF_States[0] = 1;
    EF_States[1] = 1;
    EF_States[2] = 1;
  }

  //===================== BUTTON =====================
  if (currentTime - button_sampler.lastExecutionTime > LOOP_PERIOD) {
    button_sampler.lastExecutionTime = currentTime;
    button_sampler.updateState();
  }

  //===================== IMU =====================
  if (currentTime - imu.lastExecutionTime > LOOP_PERIOD) {
    imu.lastExecutionTime = currentTime;
    imu.read();
  }

  //===================== GPS (must read every cycle) =====================
  gps.read(&GPS);

  //===================== XY STATE ESTIMATOR =====================
  if (currentTime - xy_state_estimator.lastExecutionTime > LOOP_PERIOD) {
    xy_state_estimator.lastExecutionTime = currentTime;
    xy_state_estimator.updateState(&imu.state, &gps.state);
  }

  //===================== GPS LOCK LED =====================
  if (currentTime - led.lastExecutionTime > LOOP_PERIOD) {
    led.lastExecutionTime = currentTime;
    led.flashLED(&gps.state);
  }

  //===================== LOGGER =====================
  if (currentTime - logger.lastExecutionTime > LOOP_PERIOD && logger.keepLogging) {
    logger.lastExecutionTime = currentTime;
    logger.log();
  }
}

/////////////////////////* ISRs *////////////////////////

void EFA_Detected(void) {
  EF_States[0] = 0;
}

void EFB_Detected(void) {
  EF_States[1] = 0;
}

void EFC_Detected(void) {
  EF_States[2] = 0;
}
