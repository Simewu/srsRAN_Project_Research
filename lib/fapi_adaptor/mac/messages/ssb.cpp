/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "srsgnb/fapi_adaptor/mac/messages/ssb.h"

using namespace srsgnb;
using namespace fapi;
using namespace fapi_adaptor;

/// Converts the given beta_pss_profile_nr_type value into a beta_pss_profile_type.
static beta_pss_profile_type convert_beta_pss_profile_nr(beta_pss_profile_nr_type value)
{
  switch (value) {
    case beta_pss_profile_nr_type::dB_0:
      return beta_pss_profile_type::dB_0;
    case beta_pss_profile_nr_type::dB_3:
      return beta_pss_profile_type::dB_3;
    default:
      break;
  }
  return beta_pss_profile_type::beta_pss_profile_sss;
}

void srsgnb::fapi_adaptor::convert_ssb_mac_to_fapi(fapi::dl_ssb_pdu& fapi_pdu, const srsgnb::dl_ssb_pdu& mac_pdu)
{
  dl_ssb_pdu_builder builder(fapi_pdu);

  convert_ssb_mac_to_fapi(builder, mac_pdu);
}

void srsgnb::fapi_adaptor::convert_ssb_mac_to_fapi(fapi::dl_ssb_pdu_builder& builder, const srsgnb::dl_ssb_pdu& mac_pdu)
{
  builder.set_basic_parameters(mac_pdu.pci,
                               convert_beta_pss_profile_nr(mac_pdu.beta_pss_profile_nr),
                               mac_pdu.ssb_index,
                               mac_pdu.ssb_subcarrier_offset,
                               mac_pdu.offset_to_point_A);

  builder.set_maintenance_v3_basic_parameters(mac_pdu.ssb_case, mac_pdu.scs, mac_pdu.L_max);

  optional<float> beta_pss_profile_sss;
  if (mac_pdu.beta_pss_profile_nr == beta_pss_profile_nr_type::use_profile_sss) {
    beta_pss_profile_sss.emplace(mac_pdu.beta_pss_profile_sss_in_dB);
  }

  builder.set_maintenance_v3_tx_power_info({}, beta_pss_profile_sss);

  const ssb_mib_data_pdu& mib_pdu = mac_pdu.mib_data;
  builder.set_bch_payload_phy_full(
      mib_pdu.dmrs_typeA_position, mib_pdu.pdcch_config_sib1, mib_pdu.cell_barred, mib_pdu.intra_freq_reselection);
}
