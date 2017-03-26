/* Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <debug.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <mdp5.h>

#include "gcdb_display.h"
#include "target/display.h"
#include "fastboot_oem_display.h"

struct oem_panel_data oem_data = {{'\0'}, false, false, SIM_NONE};

void panel_name_to_dt_string(struct panel_lookup_list supp_panels[],
			  uint32_t supp_panels_size,
			  const char *panel_name, const char **panel_node,
			  const char **slave_panel_node, int *panel_mode)
{
	uint32_t i;

	if (!panel_name) {
		dprintf(CRITICAL, "Invalid panel name\n");
		return;
	}

	for (i = 0; i < supp_panels_size; i++) {
		if (!strncmp(panel_name, supp_panels[i].name,
			MAX_PANEL_ID_LEN)) {
			*panel_node = supp_panels[i].panel_dt_string;
			if (supp_panels[i].slave_panel_dt_string[0]) {
				*slave_panel_node =
					supp_panels[i].slave_panel_dt_string;
				*panel_mode = DUAL_DSI_FLAG;
			} else {
				*panel_mode = 0;
			}
			break;
		}
	}

	if (i == supp_panels_size)
		dprintf(CRITICAL, "Panel_name:%s not found in lookup table\n",
			panel_name);
}

void sim_override_to_cmdline(struct sim_lookup_list sim[],
			  uint32_t sim_size, uint32_t sim_mode,
			  const char **sim_string)
{
	uint32_t i;

	for (i = 0; i < sim_size; i++) {
		if (sim_mode == sim[i].sim_mode) {
			*sim_string = sim[i].override_string;
			break;
		}
	}

	if (i == sim_size)
		dprintf(CRITICAL, "Sim_mode not found in lookup table\n");
}

struct oem_panel_data mdss_dsi_get_oem_data(void)
{
	return oem_data;
}

void set_panel_cmd_string(const char *panel_name)
{
	char *ch = NULL, *ch_hash = NULL, *ch_col = NULL;
	int i;

	panel_name += strspn(panel_name, " ");

	/* Panel string - ':' and '#' are delimiters */
	ch_col = strchr((char *) panel_name, ':');
	ch_hash = strchr((char *) panel_name, '#');

	if (ch_col && ch_hash)
		ch = (ch_col < ch_hash) ? ch_col : ch_hash;
	else if (ch_col)
		ch = ch_col;
	else if (ch_hash)
		ch = ch_hash;

	if (ch) {
		for (i = 0; (panel_name + i) < ch; i++)
			oem_data.panel[i] = *(panel_name + i);
		oem_data.panel[i] = '\0';
	} else {
		strlcpy(oem_data.panel, panel_name, MAX_PANEL_ID_LEN);
	}

	/* Skip LK configuration */
	ch = strstr((char *) panel_name, ":skip");
	oem_data.skip = ch ? true : false;

	/* Cont. splash status */
	ch = strstr((char *) panel_name, ":disable");
	oem_data.cont_splash = ch ? false : true;

	/* Simulator status */
	oem_data.sim_mode = SIM_NONE;
	if (strstr((char *) panel_name, "#sim-hwte"))
		oem_data.sim_mode = SIM_HWTE;
	else if (strstr((char *) panel_name, "#sim-swte"))
		oem_data.sim_mode = SIM_SWTE;
	else if (strstr((char *) panel_name, "#sim"))
		oem_data.sim_mode = SIM_MODE;

	/* disable cont splash when booting in simulator mode */
	if (oem_data.sim_mode)
		oem_data.cont_splash = false;

}

static bool mdss_dsi_set_panel_node(const char *panel_name, const char **dsi_id,
		const char **panel_node, const char **slave_panel_node, int *panel_mode)
{
	if (!strcmp(panel_name, SIM_VIDEO_PANEL)) {
		*dsi_id = SIM_DSI_ID;
		*panel_node = SIM_VIDEO_PANEL_NODE;
		*panel_mode = 0;
	} else if (!strcmp(panel_name, SIM_DUALDSI_VIDEO_PANEL)) {
		*dsi_id = SIM_DSI_ID;
		*panel_node = SIM_DUALDSI_VIDEO_PANEL_NODE;
		*slave_panel_node = SIM_DUALDSI_VIDEO_SLAVE_PANEL_NODE;
		*panel_mode = DUAL_DSI_FLAG;
	} else if (!strcmp(panel_name, SIM_CMD_PANEL)) {
		*dsi_id = SIM_DSI_ID;
		*panel_node = SIM_CMD_PANEL_NODE;
		*panel_mode = 0;
	} else if (!strcmp(panel_name, SIM_DUALDSI_CMD_PANEL)) {
		*dsi_id = SIM_DSI_ID;
		*panel_node = SIM_DUALDSI_CMD_PANEL_NODE;
		*slave_panel_node = SIM_DUALDSI_CMD_SLAVE_PANEL_NODE;
		*panel_mode = DUAL_DSI_FLAG;
	} else if (oem_data.skip) {
		/* For skip panel case, check the lookup table */
		*dsi_id = SIM_DSI_ID;
		panel_name_to_dt_string(lookup_skip_panels,
			ARRAY_SIZE(lookup_skip_panels), panel_name,
			panel_node, slave_panel_node, panel_mode);
	} else {
		return false;
	}
	return true;
}

bool gcdb_display_cmdline_arg(char *pbuf, uint16_t buf_size)
{
	const char *dsi_id = NULL;
	const char *panel_node = NULL;
	const char *slave_panel_node = NULL;
	const char *sim_mode_string = NULL;
	uint16_t dsi_id_len = 0, panel_node_len = 0, slave_panel_node_len = 0;
	uint32_t arg_size = 0;
	bool ret = true;
	bool rc;
	const char *default_str;
	struct panel_struct panelstruct;
	int panel_mode = SPLIT_DISPLAY_FLAG | DUAL_PIPE_FLAG | DST_SPLIT_FLAG;
	int prefix_string_len = strlen(DISPLAY_CMDLINE_PREFIX);
	const char *sctl_string;

	panelstruct = mdss_dsi_get_panel_data();

	rc = mdss_dsi_set_panel_node(oem_data.panel, &dsi_id, &panel_node,
			&slave_panel_node, &panel_mode);

	if (!rc) {
		if (panelstruct.paneldata) {
			dsi_id = panelstruct.paneldata->panel_controller;
			panel_node = panelstruct.paneldata->panel_node_id;
			panel_mode =
				panelstruct.paneldata->panel_operating_mode &
								panel_mode;
			slave_panel_node =
				panelstruct.paneldata->slave_panel_node_id;
		} else {
			if (target_is_edp())
				default_str = "0:edp:";
			else
				default_str = "0:dsi:0:";

			arg_size = prefix_string_len + strlen(default_str);
			if (buf_size < arg_size) {
				dprintf(CRITICAL, "display command line buffer is small\n");
				return false;
			}

			strlcpy(pbuf, DISPLAY_CMDLINE_PREFIX, buf_size);
			pbuf += prefix_string_len;
			buf_size -= prefix_string_len;

			strlcpy(pbuf, default_str, buf_size);
			return true;
		}
	}

	if (dsi_id == NULL || panel_node == NULL) {
		dprintf(CRITICAL, "panel node or dsi ctrl not present\n");
		return false;
	}

	if (((panel_mode & SPLIT_DISPLAY_FLAG) ||
	     (panel_mode & DST_SPLIT_FLAG)) && slave_panel_node == NULL) {
		dprintf(CRITICAL, "slave node not present in dual dsi case\n");
		return false;
	}

	dsi_id_len = strlen(dsi_id);
	panel_node_len = strlen(panel_node);
	if (!slave_panel_node || !strcmp(slave_panel_node, ""))
		slave_panel_node = NO_PANEL_CONFIG;
	slave_panel_node_len = strlen(slave_panel_node);

	arg_size = prefix_string_len + dsi_id_len + panel_node_len +
						LK_OVERRIDE_PANEL_LEN + 1;

	if (panelstruct.paneldata &&
		!strcmp(panelstruct.paneldata->panel_destination, "DISPLAY_2"))
		sctl_string = DSI_0_STRING;
	else
		sctl_string = DSI_1_STRING;

	arg_size += strlen(sctl_string) + slave_panel_node_len;

	if (oem_data.sim_mode != SIM_NONE) {
		sim_override_to_cmdline(lookup_sim,
			ARRAY_SIZE(lookup_sim), oem_data.sim_mode,
			&sim_mode_string);
		if (sim_mode_string) {
			arg_size += LK_SIM_OVERRIDE_LEN +
				strlen(sim_mode_string);
		} else {
			dprintf(CRITICAL, "SIM string NULL but mode is not NONE\n");
			return false;
		}
	}

	if (buf_size < arg_size) {
		dprintf(CRITICAL, "display command line buffer is small\n");
		ret = false;
	} else {
		strlcpy(pbuf, DISPLAY_CMDLINE_PREFIX, buf_size);
		pbuf += prefix_string_len;
		buf_size -= prefix_string_len;

		strlcpy(pbuf, LK_OVERRIDE_PANEL, buf_size);
		pbuf += LK_OVERRIDE_PANEL_LEN;
		buf_size -= LK_OVERRIDE_PANEL_LEN;

		strlcpy(pbuf, dsi_id, buf_size);
		pbuf += dsi_id_len;
		buf_size -= dsi_id_len;

		strlcpy(pbuf, panel_node, buf_size);
		pbuf += panel_node_len;
		buf_size -= panel_node_len;

		strlcpy(pbuf, sctl_string, buf_size);
		pbuf += strlen(sctl_string);
		buf_size -= strlen(sctl_string);

		strlcpy(pbuf, slave_panel_node, buf_size);
		pbuf += slave_panel_node_len;
		buf_size -= slave_panel_node_len;

		if (sim_mode_string) {
			strlcpy(pbuf, LK_SIM_OVERRIDE, buf_size);
			pbuf += LK_SIM_OVERRIDE_LEN;
			buf_size -= LK_SIM_OVERRIDE_LEN;

			strlcpy(pbuf, sim_mode_string, buf_size);
			pbuf += strlen(sim_mode_string);
			buf_size -= strlen(sim_mode_string);
		}
	}
	return ret;
}
