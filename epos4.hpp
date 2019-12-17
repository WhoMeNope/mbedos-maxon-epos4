#pragma once

#include "mbed.h"

#include "can.hpp"

#include "debug.hpp"

class Epos4 {

public:
  Epos4 (PinName rx, PinName tx);

  void startPosMode ();
  void stop ();

private:

  /* CAN */
  Thread can_listener;
  void can_handler_routine () {
    while (true) {
      // wait for + handle CAN message (state transitions)

      CANMessage msg;
      can::get(msg, osWaitForever);
      pc.printf("Got CAN message : COB-ID=%X", msg.id);

      // HEARTBEAT
      if (msg.id > 0x700) {
        uint8_t node_id = msg.id - 0x700;
        pc.printf("Got HEARTBEAT from node : %d", node_id);

        nmt_access.lock();
        if (nmt_current_state == nmt_state::Initialization)
          nmt_current_state = nmt_state::PreOperational;
        nmt_access.unlock();
      }
    }
  }

  /* NMT States

  Automatically on boot
  Initialization -> Pre-Operational

  Pre-operational
    Can be configured using SDO communication.
    Emergency objects.
    NMT Protocol to transition state.
    No PDO communication.

  Operational
    SDO, PDO, EMCY, NMT

  */
  enum nmt_state : uint8_t {
    Initialization = 0,
    PreOperational = 1,
    Operational    = 2,
    Stopped        = 3,
  };

  Mutex nmt_access;
  ConditionVariable* nmt_cond;
  nmt_state nmt_current_state;

  void block_for_nmt_state(nmt_state desired_state) {
    nmt_access.lock();
    while (nmt_current_state != desired_state) {
      nmt_cond->wait();
    }
    nmt_access.unlock();
  }
};
