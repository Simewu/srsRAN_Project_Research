/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "srsgnb/mac/mac_sdu_handler.h"
#include "srsgnb/rlc/rlc.h"

namespace srsgnb {
namespace srs_du {

class mac_sdu_rx_adapter : public mac_sdu_rx_notifier
{
public:
  explicit mac_sdu_rx_adapter(rlc_rx_pdu_handler& rlc_rx) : rlc_handler(rlc_rx) {}

  void on_new_sdu(byte_buffer_slice sdu) override { rlc_handler.handle_pdu(std::move(sdu)); }

private:
  rlc_rx_pdu_handler& rlc_handler;
};

class mac_sdu_tx_adapter : public mac_sdu_tx_builder
{
public:
  explicit mac_sdu_tx_adapter(rlc_tx_pdu_transmitter& rlc_tx) : rlc_handler(rlc_tx) {}

  byte_buffer_slice_chain on_new_tx_sdu(unsigned nof_bytes) override { return rlc_handler.pull_pdu(nof_bytes); }

  unsigned on_buffer_state_update() override { return rlc_handler.get_buffer_state(); }

private:
  rlc_tx_pdu_transmitter& rlc_handler;
};

} // namespace srs_du
} // namespace srsgnb
