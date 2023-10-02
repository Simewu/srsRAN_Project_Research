/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "si_message_scheduler.h"
#include "../support/dmrs_helpers.h"
#include "../support/pdsch/pdsch_default_time_allocation.h"
#include "../support/pdsch/pdsch_resource_allocation.h"
#include "../support/prbs_calculator.h"
#include "../support/sch_pdu_builder.h"

using namespace srsran;

si_message_scheduler::si_message_scheduler(const scheduler_si_expert_config&               expert_cfg_,
                                           const cell_configuration&                       cfg_,
                                           pdcch_resource_allocator&                       pdcch_sch_,
                                           const sched_cell_configuration_request_message& msg) :
  expert_cfg(expert_cfg_),
  cell_cfg(cfg_),
  pdcch_sch(pdcch_sch_),
  si_sched_cfg(msg.si_scheduling),
  logger(srslog::fetch_basic_logger("SCHED"))
{
  if (si_sched_cfg.has_value()) {
    pending_messages.resize(si_sched_cfg->si_messages.size());
  }
}

void si_message_scheduler::run_slot(cell_slot_resource_allocator& res_grid)
{
  if (not si_sched_cfg.has_value()) {
    return;
  }

  // Check for SI message updates
  update_si_message_windows(res_grid.slot);

  // Schedule SI messages that are within the window for tx.
  schedule_pending_si_messages(res_grid);
}

void si_message_scheduler::update_si_message_windows(slot_point sl_tx)
{
  const unsigned sfn = sl_tx.sfn();

  for (unsigned i = 0; i != pending_messages.size(); ++i) {
    const si_message_scheduling_config& si_msg = si_sched_cfg->si_messages[i];

    if (pending_messages[i].window_start.valid()) {
      // SI message is already in the window. Check for window end.
      if (pending_messages[i].window_start + si_sched_cfg->si_window_len_slots < sl_tx) {
        pending_messages[i].window_start = slot_point();
        pending_messages[i].nof_tx       = 0;
      }
      continue;
    }

    // Check for SI window start.

    // 2> for the concerned SI message, determine the number n which corresponds to the order of entry in the list of SI
    // messages configured by schedulingInfoList in si-SchedulingInfo in SIB1;
    const unsigned n = i + 1;

    // 2> determine the integer value x = (n – 1) × w, where w is the si-WindowLength
    const unsigned x = (n - 1) * si_sched_cfg->si_window_len_slots;

    // 2> the SI-window starts at the slot #a, where a = x mod N, in the radio frame for which SFN mod T = FLOOR(x/N),
    // where T is the si-Periodicity of the concerned SI message and N is the number of slots in a radio frame as
    // specified in TS 38.213
    const unsigned N = sl_tx.nof_slots_per_frame();
    const unsigned a = x % N;
    if (sl_tx.slot_index() != a) {
      continue;
    }

    const unsigned T = si_msg.period_radio_frames;
    if (sfn % T != (x / N)) {
      continue;
    }

    // SI window start detected.
    pending_messages[i].window_start = sl_tx;
    pending_messages[i].nof_tx       = 0;
  }
}

void si_message_scheduler::schedule_pending_si_messages(cell_slot_resource_allocator& res_grid)
{
  for (unsigned i = 0; i != pending_messages.size(); ++i) {
    message_window_context& si_ctxt = pending_messages[i];

    if (not si_ctxt.window_start.valid()) {
      // SI window is inactive.
      continue;
    }
  }
}

bool si_message_scheduler::allocate_si_message(unsigned si_message, cell_slot_resource_allocator& res_grid)
{
  static const unsigned time_resource = 0;
  static const unsigned nof_layers    = 1;
  // As per Section 5.1.3.2, TS 38.214, nof_oh_prb = 0 if PDSCH is scheduled by PDCCH with a CRC scrambled by SI-RNTI.
  static const unsigned nof_oh_prb = 0;

  const units::bytes si_msg_payload_size = si_sched_cfg->si_messages[si_message].msg_len;

  const auto& pdsch_td_res_alloc_list =
      get_si_rnti_type0A_common_pdsch_time_domain_list(cell_cfg.dl_cfg_common.init_dl_bwp.pdsch_common,
                                                       cell_cfg.dl_cfg_common.init_dl_bwp.generic_params.cp,
                                                       cell_cfg.dmrs_typeA_pos);
  const ofdm_symbol_range si_ofdm_symbols = pdsch_td_res_alloc_list[time_resource].symbols;
  const unsigned          nof_symb_sh     = si_ofdm_symbols.length();

  // Generate dmrs information to be passed to (i) the fnc that computes number of RE used for DMRS per RB and (ii) to
  // the fnc that fills the DCI.
  const dmrs_information dmrs_info =
      make_dmrs_info_common(pdsch_td_res_alloc_list, time_resource, cell_cfg.pci, cell_cfg.dmrs_typeA_pos);

  // Compute the number of RBs necessary for the allocation.
  const sch_mcs_description mcs_descr   = pdsch_mcs_get_config(pdsch_mcs_table::qam64, expert_cfg.si_message_mcs_index);
  const sch_prbs_tbs        si_prbs_tbs = get_nof_prbs(prbs_calculator_sch_config{si_msg_payload_size.value(),
                                                                           nof_symb_sh,
                                                                           calculate_nof_dmrs_per_rb(dmrs_info),
                                                                           nof_oh_prb,
                                                                           mcs_descr,
                                                                           nof_layers});

  // > Find available RBs in PDSCH for SI message BCCH grant.
  const search_space_id other_si_ss_id = cell_cfg.dl_cfg_common.init_dl_bwp.pdcch_common.other_si_search_space_id;
  crb_interval          si_crbs;
  {
    const crb_interval crb_lims =
        pdsch_helper::get_ra_crb_limits_common(cell_cfg.dl_cfg_common.init_dl_bwp, other_si_ss_id);
    const unsigned    nof_si_rbs = si_prbs_tbs.nof_prbs;
    const prb_bitmap& used_crbs  = res_grid.dl_res_grid.used_crbs(
        cell_cfg.dl_cfg_common.init_dl_bwp.generic_params.scs, crb_lims, si_ofdm_symbols);
    si_crbs = rb_helper::find_empty_interval_of_length(used_crbs, nof_si_rbs, 0);
    if (si_crbs.length() < nof_si_rbs) {
      // early exit
      logger.info("Skipping SI message scheduling. Cause: Not enough PDSCH space for SI Message index: {}", si_message);
      return false;
    }
  }

  // > Allocate DCI_1_0 for SI message on PDCCH.
  pdcch_dl_information* pdcch =
      pdcch_sch.alloc_dl_pdcch_common(res_grid, rnti_t::SI_RNTI, other_si_ss_id, expert_cfg.si_message_dci_aggr_lev);
  if (pdcch == nullptr) {
    logger.info("Skipping SI message scheduling. Cause: Not enough PDCCH space for SI Message index: {}", si_message);
    return false;
  }

  // > Now that we are sure there is space in both PDCCH and PDSCH, set SI CRBs as used.
  res_grid.dl_res_grid.fill(
      grant_info{cell_cfg.dl_cfg_common.init_dl_bwp.generic_params.scs, si_ofdm_symbols, si_crbs});

  // > Delegate filling SI message grants to helper function.
  fill_si_grant();
  return true;
}

void si_message_scheduler::fill_si_grant()
{
  // TODO
}
