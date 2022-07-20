/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "mac_impl.h"
#include "mac_ctrl/srs_sched_config_adapter.h"
#include "srsgnb/scheduler/scheduler_factory.h"

namespace srsgnb {

mac_impl::mac_impl(mac_ul_ccch_notifier&     event_notifier,
                   du_l2_ul_executor_mapper& ul_exec_mapper_,
                   du_l2_dl_executor_mapper& dl_exec_mapper_,
                   task_executor&            ctrl_exec_,
                   mac_result_notifier&      phy_notifier_) :
  cfg(event_notifier, ul_exec_mapper_, dl_exec_mapper_, ctrl_exec_, phy_notifier_),
  sched_cfg_adapter(cfg),
  sched_obj(create_scheduler(sched_cfg_adapter.get_sched_notifier())),
  dl_unit(cfg, *sched_obj, *sched_obj, rnti_table),
  ul_unit(cfg, *sched_obj, rnti_table),
  ctrl_unit(cfg, ul_unit, dl_unit, rnti_table, sched_cfg_adapter),
  rach_hdl(*sched_obj, rnti_table)
{
  sched_cfg_adapter.set_sched(*sched_obj);
}

void mac_impl::handle_dl_bsr_update_required(const mac_dl_bsr_indication_message& dl_bsr)
{
  dl_bsr_indication_message bsr{};
  bsr.ue_index = dl_bsr.ue_index;
  bsr.lcid     = dl_bsr.lcid;
  bsr.bsr      = dl_bsr.bsr;
  sched_obj->handle_dl_bsr_indication(bsr);
}

} // namespace srsgnb
