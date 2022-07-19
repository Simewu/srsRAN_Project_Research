/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "rlc_am_pdu.h"

#pragma once

namespace srsgnb {

/// Provides access to status functions of the RLC RX AM entity.
/// The RLC TX AM entity uses this class to
/// - get status PDUs for transmission to lower layers
/// - get length of status PDUs for buffer state calculation
/// - check whether a status report has been polled by the peer
class rlc_rx_am_status_provider
{
public:
  virtual ~rlc_rx_am_status_provider() = default;

  virtual rlc_am_status_pdu get_status_pdu()         = 0;
  virtual uint32_t          get_status_pdu_length()  = 0;
  virtual bool              status_report_required() = 0;
};

/// This interface represents the status PDU entry point of a RLC TX AM entity.
/// The RLC RX AM entity uses this class to forward received status PDUs to the RLC TX AM entity.
class rlc_tx_am_status_handler
{
public:
  virtual ~rlc_tx_am_status_handler() = default;

  virtual void handle_status_pdu(rlc_am_status_pdu status_pdu) = 0;
};

} // namespace srsgnb
