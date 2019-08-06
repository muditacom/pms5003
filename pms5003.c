#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <byteswap.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>

#define T_VERSION 0
FILE *g_log, *g_fd;

#if T_VERSION == 0 // non-T sensor version
  #define DATA11_STRING "count_5um"
  #define DATA12_STRING "count_10um"
#else // T sensor version (measures temperature and humidity)
  #define DATA11_STRING "temperature"
  #define DATA12_STRING "humidity"
#endif

void set_interface_attribs(int fd, int speed)
{
  struct termios tty;

  if (tcgetattr(fd, &tty) < 0) {
    fprintf(stderr, "fatal: tcgetattr(): %s\n", strerror(errno));
    exit(-1);
  }

  cfsetospeed(&tty, (speed_t) speed);
  cfsetispeed(&tty, (speed_t) speed);

  tty.c_cflag |= (CLOCAL | CREAD);      /* ignore modem controls */
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;           /* 8-bit characters */
  tty.c_cflag &= ~PARENB;       /* no parity bit */
  tty.c_cflag &= ~CSTOPB;       /* only need 1 stop bit */
  tty.c_cflag &= ~CRTSCTS;      /* no hardware flowcontrol */

  /* setup for non-canonical mode */
  tty.c_iflag &=
      ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  tty.c_oflag &= ~OPOST;

  /* fetch bytes as they become available */
  tty.c_cc[VMIN] = 1;
  tty.c_cc[VTIME] = 1;

  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    fprintf(stderr, "fatal: tcsetattr(): %s\n", strerror(errno));
    exit(-1);
  }
}

void forced_read(int fd, void *buf, size_t count)
{
  size_t num_read = 0;

  do {
    int rdlen = read(fd, (char *) buf + num_read, count - num_read);
    if (rdlen < 1) {
      fprintf(stderr,
              "fatal: read() == %d (got %lu of %lu requested): %s\n",
              rdlen, num_read, count, strerror(errno));
      exit(-1);
    }
    num_read += rdlen;
  } while (num_read < count);
}

time_t start_time;

void print_wrap(char* fmt, ...)
{
    char* str=NULL;
   va_list args;
   va_start(args, fmt);
   int ret=vasprintf(&str,fmt, args);
   va_end(args);

   if(g_log != NULL) {
       fprintf(g_log,"%s\n", str);
   }

   if(g_fd != NULL) {
       fprintf(g_fd,"%s\n", str);
   }
   if(ret<=0) {
       fprintf(stderr,"vasprintf error! %s\n", strerror(errno));
   }
   fprintf(stdout,"%s\n", str);
   free(str);
}

void print_data(double *data)
{
  static const char *labels[] = { "std_pm1", "std_pm2_5", "std_pm10",
    "atm_pm1", "atm_pm2_5", "atm_pm10",
    "count_0_3um", "count_0_5um", "count_1um",
    "count_2_5um", DATA11_STRING, DATA12_STRING
  };

  time_t current_time;
  time(&current_time);
  double diff_time = difftime(current_time, start_time);

  print_wrap("{\"t\": %lu, \n\"dt\":%.1f", current_time, diff_time);

#if T_VERSION == 1
  data[10] = data[10] / 10.0;
  data[11] = data[11] / 10.0;
#endif

  for (size_t i = 3; i < 12; i++) {
    print_wrap(",\"%s\":%.02f", labels[i], data[i]);
  }

  print_wrap("}\n");
}

double avg_sum[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

size_t avg_num = 0;

void kill_handler(int i)
{
    printf("Kill %d!\n", i);
    if(g_log != 0) {
        fflush(g_log);
        fclose(g_log);
        printf("Close log\n");
    }
    if(g_fd != 0) {
        fflush(g_fd);
        fclose(g_fd);
        printf("Close fd\n");
    }
}
void sigalrm_handler()
{
  if (avg_num > 0) {
    for (size_t i = 0; i < sizeof(avg_sum) / sizeof(avg_sum[0]); i++)
      avg_sum[i] /= avg_num;
    print_data(avg_sum);
    exit(0);
  } else {
    fprintf(stderr,
            "warning: no data frames collected in the given time span.\n");
    exit(-1);
  }
}

int main(int argc, char **argv) {
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "usage: %s <port> [ <average-over-this-many-seconds>]\n", argv[0]);
        exit(-1);
    }

    signal(SIGKILL, kill_handler);
    signal(SIGINT, kill_handler);

    size_t average_over = 0;
    if (argc == 3) {
        average_over = atoi(argv[2]);
        signal(SIGALRM, sigalrm_handler);
        alarm(average_over);
    }

    const char *portname = argv[1];
    int fd = 0;

    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "fatal: open(): %s: %s\n", portname, strerror(errno));
        exit(-1);
    }
    g_log = fopen("./log.log", "w+");
    if (g_log == NULL) {
        printf("Error: ./log.log %s\n", strerror(errno));
    }
    g_fd = fdopen(fd, "rw+");
    if (g_fd == NULL) {
        printf("Error: %s %s\n", portname, strerror(errno));
    }
    print_wrap("LOG START\n");

    set_interface_attribs(fd, B9600);
    time(&start_time);
    for (;;) {
        char ch;
        forced_read(fd, &ch, 1);
        if (ch != 0x42)
            continue;
        forced_read(fd, &ch, 1);
        if (ch != 0x4d)
            continue;

        unsigned short data[15];
        forced_read(fd, &data, sizeof(data));
        for (size_t i = 0; i < sizeof(data) / sizeof(short); i++)
            data[i] = __bswap_16(data[i]);

        // Check frame length.
        if (data[0] != 2 * 13 + 2)
            continue;

        // Check sum.
        size_t check_sum = 0x42 + 0x4d;
        for (size_t i = 0; i < sizeof(data) - sizeof(data[0]); i++)
            check_sum += ((unsigned char *)data)[i];
        if (check_sum != data[14])
            continue;

        double data_fp[12];
        for (size_t i = 0; i < sizeof(avg_sum) / sizeof(avg_sum[0]); i++) {
            data_fp[i] = data[i + 1];
            avg_sum[i] += data_fp[i];
        }
        avg_num++;

        // Print each frame if not averaging over time.
        if (!average_over)
            print_data(data_fp);
    }

    close(fd); // Won't happen...
}
