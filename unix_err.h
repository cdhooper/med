#ifndef UNIX_ERR_H
#define UNIX_ERR_H

void errx(int ec, const char *fmt, ...);
void err(int ec, const char *fmt, ...);
void warnx(const char *fmt, ...);
void warn(const char *fmt, ...);

#endif /* UNIX_ERR_H */
