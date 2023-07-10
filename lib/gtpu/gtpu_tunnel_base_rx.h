/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "gtpu_pdu.h"
#include "gtpu_tunnel_logger.h"
#include "srsran/adt/byte_buffer.h"
#include "srsran/gtpu/gtpu_config.h"
#include "srsran/gtpu/gtpu_tunnel_rx.h"
#include <cstdint>

namespace srsran {

/// Class used for receiving GTP-U bearers.
class gtpu_tunnel_base_rx : public gtpu_tunnel_rx_upper_layer_interface
{
public:
  gtpu_tunnel_base_rx(uint32_t ue_index, gtpu_config::gtpu_rx_config cfg_) :
    logger("GTPU", {ue_index, cfg_.local_teid, "DL"}), cfg(cfg_)
  {
    // Validate configuration
    logger.log_info("GTPU configured. {}", cfg);
  }
  ~gtpu_tunnel_base_rx() = default;

  /*
   * SDU/PDU handlers
   */
  void handle_pdu(byte_buffer buf) final
  {
    gtpu_dissected_pdu dissected_pdu;
    bool               read_ok = gtpu_dissect_pdu(dissected_pdu, std::move(buf), logger);
    if (!read_ok) {
      logger.log_error("Dropped PDU, error reading GTP-U header. pdu_len={}", dissected_pdu.buf.length());
      return;
    }
    if (dissected_pdu.hdr.teid != cfg.local_teid) {
      logger.log_error(
          "Dropped PDU, mismatched TEID. pdu_len={} teid={:#x}", dissected_pdu.buf.length(), dissected_pdu.hdr.teid);
      return;
    }

    // continue processing in domain-specific subclass
    handle_pdu(std::move(dissected_pdu));
  }

protected:
  virtual void handle_pdu(gtpu_dissected_pdu&& pdu) = 0;

  gtpu_tunnel_logger                logger;
  const gtpu_config::gtpu_rx_config cfg;
};
} // namespace srsran
