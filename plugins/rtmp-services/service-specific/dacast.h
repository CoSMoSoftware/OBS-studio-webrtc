#pragma once

struct dacast_ingest {
	const char *url;
	const char *username;
	const char *password;
	const char *streamkey;
};

extern void init_dacast_data(void);
extern void unload_dacast_data(void);

extern void dacast_ingests_load_data(const char *server, const char *key);
extern struct dacast_ingest *dacast_ingest(const char *key);
