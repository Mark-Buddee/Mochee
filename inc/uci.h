#ifndef UCI_H
#define UCI_H

// Defining how much of a time lead we have
#define LARGE_LEAD 15000
#define LEAD       5000

// Defining time controls
#define RAPID 300000 // 5 min+
#define BLITZ 120000 // 2 min+

// Defining move times
#define RAPID_MAX  30000
#define BLITZ_MAX  10000
#define BULLET_MAX 2000
#define RAPID_STANDARD  15000
#define BLITZ_STANDARD  3000
#define BULLET_STANDARD 1000
#define RAPID_RUSH  5000
#define BLITZ_RUSH  2000
#define BULLET_RUSH 500
#define DANGER 100
#define INSTANT 0

void uci(void);

#endif