/*
 * Copyright (c) 2002-2013, 2015 Balabit
 * Copyright (c) 1998-2013, 2015 Balázs Scheidler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include "stateful-parser.h"
#include <string.h>

void
stateful_parser_set_inject_mode(StatefulParser *self, LogDBParserInjectMode inject_mode)
{
  self->inject_mode = inject_mode;
}

void
stateful_parser_emit_synthetic(StatefulParser *self, LogMessage *msg)
{
  if (self->inject_mode != LDBP_IM_INTERNAL)
    {
      LogPathOptions path_options = LOG_PATH_OPTIONS_INIT;

      path_options.ack_needed = FALSE;
      log_pipe_forward_msg(&self->super.super, log_msg_ref(msg), &path_options);
    }
  else
    {
      msg_post_message(log_msg_ref(msg));
    }
}

void
stateful_parser_emit_synthetic_list(StatefulParser *self, LogMessage **values, gsize len)
{
  for (gint i = 0; i < len; i++)
    {
      LogMessage *msg = values[i];
      stateful_parser_emit_synthetic(self, msg);
      log_msg_unref(msg);
    }
}

void
_queue(LogPipe *s, LogMessage *msg, const LogPathOptions *path_options)
{
  StatefulParser *self = (StatefulParser *) s;
  LogPathOptions local_path_options = *path_options;
  gboolean matched = TRUE;

  /* if we consume messages into our state, then let's consider these
   * messages matched, even though we are dropping them */

  local_path_options.matched = &matched;
  log_parser_queue_method(s, msg, &local_path_options);

  /* propagate our "matched" state */
  if (path_options->matched && !matched && self->inject_mode != LDBP_IM_AGGREGATE_ONLY)
    *path_options->matched = FALSE;
}

void
stateful_parser_init_instance(StatefulParser *self, GlobalConfig *cfg)
{
  log_parser_init_instance(&self->super, cfg);
  self->super.super.queue = _queue;
  self->inject_mode = LDBP_IM_PASSTHROUGH;
}

void
stateful_parser_free_method(LogPipe *s)
{
  log_parser_free_method(s);
}

int
stateful_parser_lookup_inject_mode(const gchar *inject_mode)
{
  if (strcasecmp(inject_mode, "internal") == 0)
    return LDBP_IM_INTERNAL;
  else if (strcasecmp(inject_mode, "pass-through") == 0 || strcasecmp(inject_mode, "pass_through") == 0)
    return LDBP_IM_PASSTHROUGH;
  else if (strcasecmp(inject_mode, "aggregate-only") == 0 || strcasecmp(inject_mode, "aggregate_only") == 0)
    return LDBP_IM_AGGREGATE_ONLY;
  return -1;
}
