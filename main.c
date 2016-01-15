#include <ffdl.h>
#include <stdio.h>
#include <argp.h>
#include <string.h>

struct arguments {
  char *url;
  char *file;
  long maxNumConns;
  unsigned long long chunksizeBytes;
  long timeoutSecs;
  long rateLimit;
};

//
// ARGP options
//
static struct argp_option options[] = {
    {"file", 'f', "FILE", 0, "Local file to download to."},
    {"max-connections", 'm', "NUM_CONNECTIONS", 0,
     "Maximum number of simultaneous connections when downloading."},
    {"chunk-size", 's', "SIZE", 0, "Size in bytes of the chunks of data."},
    {"timeout", 't', "TIMEOUT", 0, "Connection timeout in seconds."},
    {"rate-limit", 'r', "LIMIT", 0, "Download rate limit in bytes per second."},
    {0}};

static char args_doc[] = "URL";

static char doc[] = "ffdl -- Fast file downloader. Downloads a file from a "
                    "given url using multiple HTTP connections in parallel";

// Gets the filename to download to from a url.
// Returns a pointer to the start of the filename. The returned char * will
// always
//   point to a character in the parameter url.
static char *get_filename_from_url(char *url) {
  char *filename = strrchr(url, '/');

  // if there is no path, just use the full url
  if (filename == NULL) {
    filename = url;
  } else {
    ++filename;
    if (filename == '\0') {
      filename = url;
    }
  }

  return filename;
}

// Parser for argp
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  long maxNumConns;
  long timeoutSecs;
  long rateLimit;
  unsigned long long chunksizeBytes;
  switch (key) {
  case 'f':
    arguments->file = arg;
    break;
  case 'm':
    sscanf(arg, "%ld", &maxNumConns);
    arguments->maxNumConns = maxNumConns;
    break;
  case 's':
    sscanf(arg, "%llu", &chunksizeBytes);
    arguments->chunksizeBytes = chunksizeBytes;
    break;
  case 't':
    sscanf(arg, "%ld", &timeoutSecs);
    arguments->timeoutSecs = timeoutSecs;
    break;
  case 'r':
    sscanf(arg, "%ld", &rateLimit);
    arguments->rateLimit = rateLimit;
    break;
  case ARGP_KEY_ARG:
    if (state->arg_num >= 1) {
      argp_usage(state);
    }
    arguments->url = arg;
    break;
  case ARGP_KEY_END:
    if (state->arg_num < 1) {
      argp_usage(state);
    }
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char *argv[]) {
  struct arguments arguments;
  memset(&arguments, 0, sizeof(arguments));

  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  if (arguments.file == '\0') {
    arguments.file = get_filename_from_url(arguments.url);
  }

  printf("Downloading \"%s\" to \"%s\"\n", arguments.url, arguments.file);

  if (ffdl_download_to_file_with_options(
          arguments.url, arguments.file, arguments.chunksizeBytes,
          arguments.maxNumConns, arguments.timeoutSecs,
          arguments.rateLimit) == 1) {
    // success!
    return 0;
  }

  // failed to download file
  return 2;
}
