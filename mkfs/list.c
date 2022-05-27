/**
 * @file link.c
 * @author Wang Haibo (wanghb@bupt.com)
 * @brief List information about sepecified files.
 * @version 0.1
 * @date 2022-04-22
 *
 * @copyright Copyright (c) 2022
 */

#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

bool list_recursive = false, list_all = false;
off_t lowest_size = 0, highest_size = LLONG_MAX;
int mdays = -1;

const struct option longopts[] = {
    {"help", no_argument, 0, 0},
    {"all", no_argument, 0, 'a'},
    {"recursive", no_argument, 0, 'r'},
    {"low", required_argument, 0, 'l'},
    {"high", required_argument, 0, 'h'},
    {"mdays", required_argument, 0, 'm'},
    {0, 0, 0, 0},
};

const char optstring[] = "arl:h:m:";

void print_usage() {
  printf(
      "BUPT Linux Course Homework #2\n"
      "Wang Haibo 2019211479\n\n"
      "Usage: ./list [OPTION ...] [FILE...]\n\n"
      "List information about FILEs (FILEs can be directories).\n"
      "If not specify FILE, use current directory as default.\n\n"
      "    --help             Display usage and exit\n"
      "-a, --all              Display all entries, including files starting "
      "with .\n"
      "-r, --recursive        List subdirectories recursively\n"
      "-l, --low <bytes>      Minimus of file size\n"
      "-h, --high <bytes>     Maximum of file size\n"
      "-m, --mdays <ndays>    Limit file last modified time");
}

void bytes_to_human_readable(off_t bytes, char *str) {
  if (bytes < 0) return;
  long long gb = bytes >> 30, mb = bytes >> 20, kb = bytes >> 10;
  if (gb)
    sprintf(str, "%.2f GB", (double)bytes / (1 << 30));
  else if (mb)
    sprintf(str, "%.2f MB", (double)bytes / (1 << 20));
  else if (kb)
    sprintf(str, "%.2f KB", (double)bytes / (1 << 10));
  else
    sprintf(str, "%d B", (int)bytes);
}

void _list(const char *path, bool _list_recursive) {
  struct stat buf;

  if (stat(path, &buf) == 0) {
    if (!list_all && (basename((char *)path)[0] == '.')) return;
    if (buf.st_mode & S_IFREG) {
      if (buf.st_size < lowest_size || buf.st_size > highest_size) return;
      time_t current_time;
      time(&current_time);
      int days = (current_time - buf.st_mtime) / (24 * 60 * 60);
      if (mdays != -1 && days > mdays) return;
      char sizestr[20];
      bytes_to_human_readable(buf.st_size, sizestr);
      printf("%12s %s\n", sizestr, path);
    } else if (buf.st_mode & S_IFDIR) {
      if (!_list_recursive) return;
      DIR *dir;
      struct dirent *entry;
      if ((dir = opendir(path)) == NULL) {
        printf("Error %d while listing %s: %s\n", errno, path, strerror(errno));
        exit(EXIT_FAILURE);
      }
      while ((entry = readdir(dir)) != NULL) {
        char bufpath[512];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
          continue;
        sprintf(bufpath, "%s%s%s", path,
                path[strlen(path) - 1] == '/' ? "" : "/", entry->d_name);
        _list(bufpath, list_recursive);
      }
      closedir(dir);
    }
  } else {
    printf("Error %d while listing %s: %s\n", errno, path, strerror(errno));
  }
}

void list(const char *path) { _list(path, true); }

int main(int argc, char *argv[]) {
  // read command options
  int ch;             // getopt result
  int opt_index = 0;  // option index in longopts

  while ((ch = getopt_long(argc, argv, optstring, longopts, &opt_index)) !=
         -1) {
    switch (ch) {
      case 0:
        if (strcmp(longopts[opt_index].name, "help") == 0) {
          print_usage();
          return 0;
        } else {
          printf("error: invalid long option!\n");
          print_usage();
          return 1;
        }
        break;
      case 'a':
        list_all = true;
        break;
      case 'r':
        list_recursive = true;
        break;
      case 'l':
        lowest_size = atoi(optarg);
        if (lowest_size < 0) {
          printf("error: negative lowest size!\n");
          print_usage();
          exit(EXIT_FAILURE);
        }
        break;
      case 'h':
        highest_size = atoi(optarg);
        if (highest_size < 0) {
          printf("error: negative highest size!\n");
          print_usage();
          return 1;
        }
        break;
      case 'm':
        mdays = atoi(optarg);
        if (mdays < 0) {
          printf("error: negative mdays!\n");
          print_usage();
          return 1;
        }
        break;
      default:
        print_usage();
        return 0;
    }
  }

  if (optind < argc) {
    // list files
    while (optind < argc) list(argv[optind++]);
  } else {
    list("./");
  }

  return 0;
}
