#include "ftl.h"
#include "ftl_private.h"
#ifndef DISABLE_AUTO_INGEST
#include <curl/curl.h>
#include <jansson.h>
#endif

static int _ingest_lookup_ip(const char *ingest_location, char ***ingest_ip);
static int _ping_server(const char *ip, int port);
OS_THREAD_ROUTINE _ingest_get_rtt(void *data);

typedef struct {
    ftl_ingest_t *ingest;
    ftl_stream_configuration_private_t *ftl;
}_tmp_ingest_thread_data_t;

static int _ping_server(const char *hostname, int port) {

  SOCKET sock;
  struct addrinfo hints;
  char dummy[4];
  struct timeval start, stop, delta;
  int retval = -1;
  struct addrinfo* resolved_names = 0;
  struct addrinfo* p = 0;
  int err = 0;
  int off = 0;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = 0;

  int ingest_port = INGEST_PORT;
  char port_str[10];

  snprintf(port_str, 10, "%d", port);
  
   err = getaddrinfo(hostname, port_str, &hints, &resolved_names);
  if (err != 0) {
    return FTL_DNS_FAILURE;
  }
  
  do {
    for (p = resolved_names; p != NULL; p = p->ai_next) {
      sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (sock == -1) {
        continue;
      }

      setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off));
      set_socket_recv_timeout(sock, 500);

      gettimeofday(&start, NULL);

      if (sendto(sock, dummy, sizeof(dummy), 0, p->ai_addr, (int)p->ai_addrlen) == SOCKET_ERROR) {
        printf("Sendto error: %s\n", get_socket_error());
        break;
      }

      if (recv(sock, dummy, sizeof(dummy), 0) < 0) {
        break;
      }

      gettimeofday(&stop, NULL);
      timeval_subtract(&delta, &stop, &start);
      retval = (int)timeval_to_ms(&delta);
    }
  } while (0);

  /* Free the resolved name struct */
  freeaddrinfo(resolved_names);

  shutdown_socket(sock, SD_BOTH);
  close_socket(sock);

  return retval;
}

OS_THREAD_ROUTINE _ingest_get_rtt(void *data) {
    _tmp_ingest_thread_data_t *thread_data = (_tmp_ingest_thread_data_t *)data;
    ftl_stream_configuration_private_t *ftl = thread_data->ftl;
    ftl_ingest_t *ingest = thread_data->ingest;
    int ping;

    ingest->rtt = 1000;

    if ((ping = _ping_server(ingest->hostname, INGEST_PING_PORT)) >= 0) {
        ingest->rtt = ping;
    }

    return 0;
}

#ifndef DISABLE_AUTO_INGEST
OS_THREAD_ROUTINE _ingest_get_hosts(ftl_stream_configuration_private_t *ftl);

static size_t _curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

OS_THREAD_ROUTINE _ingest_get_hosts(ftl_stream_configuration_private_t *ftl) {
  CURL *curl_handle;
  CURLcode res;
  struct MemoryStruct chunk;
  char *query_result = NULL;
  size_t i = 0;
  int total_ingest_cnt = 0;
  json_error_t error;
  json_t *ingests = NULL, *ingest_item = NULL;

  curl_handle = curl_easy_init();

  chunk.memory = malloc(1);  /* will be grown as needed by realloc */
  chunk.size = 0;    /* no data at this point */

  curl_easy_setopt(curl_handle, CURLOPT_URL, INGEST_LIST_URI);
  curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, TRUE);
  curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 2L);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, _curl_write_callback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "ftlsdk/1.0");

#if LIBCURL_VERSION_NUM >= 0x072400
  // A lot of servers don't yet support ALPN
  curl_easy_setopt(curl_handle, CURLOPT_SSL_ENABLE_ALPN, 0);
#endif

  res = curl_easy_perform(curl_handle);

  if (res != CURLE_OK) {
    printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    goto cleanup;
  }

  if ((ingests = json_loadb(chunk.memory, chunk.size, 0, &error)) == NULL) {
    goto cleanup;
  }
  
  size_t size = json_array_size(ingests);

  for (i = 0; i < size; i++) {
    char *name = NULL, *ip=NULL, *hostname=NULL;
    ingest_item = json_array_get(ingests, i);
  if (json_unpack(ingest_item, "{s:s, s:s, s:s}", "name", &name, "ip", &ip, "hostname", &hostname) < 0) {
    continue;
  }

    ftl_ingest_t *ingest_elmt;

    if ((ingest_elmt = malloc(sizeof(ftl_ingest_t))) == NULL) {
      goto cleanup;
    }

  ingest_elmt->name = _strdup(name);
  ingest_elmt->ip = _strdup(ip);
  ingest_elmt->hostname = _strdup(hostname);

    ingest_elmt->rtt = 500;

    ingest_elmt->next = NULL;

    if (ftl->ingest_list == NULL) {
      ftl->ingest_list = ingest_elmt;
    }
    else {
      ftl_ingest_t *tail = ftl->ingest_list;
      while (tail->next != NULL) {
        tail = tail->next;
      }

      tail->next = ingest_elmt;
    }

    total_ingest_cnt++;
  }

cleanup:
  free(chunk.memory);
  curl_easy_cleanup(curl_handle);
  if (ingests != NULL) {
    json_decref(ingests);
  }

  ftl->ingest_count = total_ingest_cnt;

  return total_ingest_cnt;
}

char * ingest_find_best(ftl_stream_configuration_private_t *ftl) {

  OS_THREAD_HANDLE *handle;
  _tmp_ingest_thread_data_t *data;
  int i;
  ftl_ingest_t *elmt, *best = NULL;
  struct timeval start, stop, delta;

  /*get list of ingest each time as they are dynamically selected*/
  while (ftl->ingest_list != NULL) {
    elmt = ftl->ingest_list;
    ftl->ingest_list = elmt->next;
  free(elmt->hostname);
  free(elmt->ip);
  free(elmt->name);
    free(elmt);
  }

  if (_ingest_get_hosts(ftl) <= 0) {
    return NULL;
  }

  if ((handle = (OS_THREAD_HANDLE *)malloc(sizeof(OS_THREAD_HANDLE) * ftl->ingest_count)) == NULL) {
    return NULL;
  }

  if ((data = (_tmp_ingest_thread_data_t *)malloc(sizeof(_tmp_ingest_thread_data_t) * ftl->ingest_count)) == NULL) {
    return NULL;
  }

  gettimeofday(&start, NULL);

  /*query all the ingests about cpu and rtt*/
  elmt = ftl->ingest_list;
  for (i = 0; i < ftl->ingest_count && elmt != NULL; i++) {
    handle[i] = 0;
    data[i].ingest = elmt;
    data[i].ftl = ftl;
    os_create_thread(&handle[i], NULL, _ingest_get_rtt, &data[i]);
    sleep_ms(5); //space out the pings
    elmt = elmt->next;
  }

  /*wait for all the ingests to complete*/
  elmt = ftl->ingest_list;
  for (i = 0; i < ftl->ingest_count && elmt != NULL; i++) {

    if (handle[i] != 0) {
      os_wait_thread(handle[i]);
    }

    if (best == NULL || elmt->rtt < best->rtt) {
      best = elmt;
    }

    elmt = elmt->next;
  }

  gettimeofday(&stop, NULL);
  timeval_subtract(&delta, &stop, &start);
  int ms = (int)timeval_to_ms(&delta);

  FTL_LOG(ftl, FTL_LOG_INFO, "It took %d ms to query all ingests\n", ms);

  elmt = ftl->ingest_list;
  for (i = 0; i < ftl->ingest_count && elmt != NULL; i++) {
    if (handle[i] != 0) {
      os_destroy_thread(handle[i]);
    }

    elmt = elmt->next;
  }

  free(handle);
  free(data);

  if (best){
    FTL_LOG(ftl, FTL_LOG_INFO, "%s at ip %s had the shortest RTT of %d ms\n", best->hostname, best->ip, best->rtt);
    return _strdup(best->hostname);
  }

  return NULL;
}
#endif

void ingest_release(ftl_stream_configuration_private_t *ftl) {

  ftl_ingest_t *elmt, *tmp;

  elmt = ftl->ingest_list;

  while (elmt != NULL) {
    tmp = elmt->next;
    free(elmt);
    elmt = tmp;
  }
}
