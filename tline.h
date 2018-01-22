/*
 * qtripp
 * Copyright (C) 2017 Jan-Piet Mens <jp@mens.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


char *handle_report(struct udata *ud, char *line, char **resp);
int handle_file_reports(struct udata *ud, FILE *fp);
void pub(struct udata *ud, char *topic, char *payload, bool retain);
void print_stats(struct udata *ud);
void dump_stats(struct udata *ud);
void pong(struct udata *ud, char *topic);
void pseudo_lwt(struct udata *ud, char *imei);
