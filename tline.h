
char *handle_report(struct udata *ud, char *line);
void pub(struct udata *ud, char *topic, char *payload, bool retain);
void print_stats(struct udata *ud);
void dump_stats(struct udata *ud);
